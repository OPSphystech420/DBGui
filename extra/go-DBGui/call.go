package main

import (
    "database/sql"
    "encoding/json"
    "fmt"
    "log"
    "net/http"
    "regexp"
    "strings"
)

func fail(w http.ResponseWriter, status int, format string, a ...interface{}) {
    msg := fmt.Sprintf(format, a...)
    http.Error(w, msg, status)
    log.Printf("[callHandler] %d %s", status, msg)
}

var validProc = regexp.MustCompile(`^[a-zA-Z0-9_.]+$`)

func callHandler(w http.ResponseWriter, r *http.Request) {
    if r.Method != http.MethodPost {
        fail(w, http.StatusMethodNotAllowed, "Only POST method is allowed")
        return
    }
    if err := r.ParseForm(); err != nil {
        fail(w, http.StatusBadRequest, "Error parsing form: %v", err)
        return
    }

    routine := r.FormValue("routine")
    if routine == "" {
        fail(w, http.StatusBadRequest, "`routine` parameter is required")
        return
    }
    if !validProc.MatchString(routine) {
        fail(w, http.StatusBadRequest, "invalid routine name: %q", routine)
        return
    }

    const metaSQL = `
        SELECT parameter_name, parameter_mode
          FROM information_schema.parameters
         WHERE specific_schema = SUBSTRING_INDEX(?,'.',1)
           AND specific_name   = SUBSTRING_INDEX(?,'.',-1)
         ORDER BY ordinal_position`
    rows, err := db.Query(metaSQL, routine, routine)
    if err != nil {
        fail(w, http.StatusInternalServerError, "Meta query error: %v", err)
        return
    }
    type param struct {
        name string
        mode string // IN | OUT | INOUT
    }
    var params []param
    for rows.Next() {
        var p param
        _ = rows.Scan(&p.name, &p.mode)
        if p.mode == "IN" || p.mode == "INOUT" {
            params = append(params, p)
        }
    }
    rows.Close()

    placeholders := make([]string, len(params))
    vals         := make([]interface{}, len(params))
    for i, p := range params {
        v := r.Form.Get(p.name)
        if v == "" && p.mode == "IN" {         // IN param missing?
            msg := fmt.Sprintf("missing required param: %s", p.name)
            http.Error(w, msg, http.StatusBadRequest)
            return
        }
        placeholders[i] = "?"
        vals[i]         = v
    }

    typ := "PROCEDURE"
    if isFunc, _ := isFunction(routine); isFunc {
        typ = "FUNCTION"
    }

    var stmt string
    if typ == "PROCEDURE" {
        stmt = fmt.Sprintf("CALL %s(%s)", routine, strings.Join(placeholders, ","))
    } else { // FUNCTION
        stmt = fmt.Sprintf(`SELECT %s(%s) AS _result`, routine, strings.Join(placeholders, ","))
    }
    log.Println("Executing:", stmt, "values:", vals)

    rs, err := db.Query(stmt, vals...)
    if err != nil {
        http.Error(w, "Routine call error: "+err.Error(), http.StatusInternalServerError)
        return
    }
    defer rs.Close()

    var allSets [][]map[string]interface{}
    for {
        set, err := scanResultSet(rs)
        if err != nil {
            http.Error(w, "Row scan error: "+err.Error(), http.StatusInternalServerError)
            return
        }
        if len(set) > 0 {
            allSets = append(allSets, set)
        }
        if !rs.NextResultSet() {
            break
        }
    }

    w.Header().Set("Content-Type", "application/json")
    switch len(allSets) {
    case 0:
        _ = json.NewEncoder(w).Encode(map[string]string{"status": "ok"})
    case 1:
        _ = json.NewEncoder(w).Encode(allSets[0])
    default:
        _ = json.NewEncoder(w).Encode(allSets)
    }
}

func isFunction(routine string) (bool, error) {
    const q = `
        SELECT routine_type
          FROM information_schema.routines
         WHERE routine_schema = SUBSTRING_INDEX(?,'.',1)
           AND routine_name   = SUBSTRING_INDEX(?,'.',-1)
         LIMIT 1`
    var rtype string
    err := db.QueryRow(q, routine, routine).Scan(&rtype)
    if err != nil {
        return false, err
    }
    return rtype == "FUNCTION", nil
}

// func scanResultSet(rows *sql.Rows) ([]map[string]interface{}, error) {
//     cols, err := rows.Columns()
//     if err != nil {
//         return nil, err
//     }
//     var out []map[string]interface{}
//     for rows.Next() {
//         vals := make([]interface{}, len(cols))
//         ptrs := make([]interface{}, len(cols))
//         for i := range vals {
//             ptrs[i] = &vals[i]
//         }
//         if err := rows.Scan(ptrs...); err != nil {
//             return nil, err
//         }
//         rec := make(map[string]interface{})
//         for i, c := range cols {
//             rec[c] = vals[i]
//         }
//         out = append(out, rec)
//     }
//     return out, rows.Err()
// }

func scanResultSet(rows *sql.Rows) ([]map[string]interface{}, error) {
    cols, err := rows.Columns()
    if err != nil {
        return nil, err
    }
    var out []map[string]interface{}
    for rows.Next() {
        vals := make([]interface{}, len(cols))
        ptrs := make([]interface{}, len(cols))
        for i := range vals {
            ptrs[i] = &vals[i]
        }
        if err := rows.Scan(ptrs...); err != nil {
            return nil, err
        }
        rec := make(map[string]interface{}, len(cols))
        for i, c := range cols {
            v := vals[i]
            if b, ok := v.([]byte); ok {
                rec[c] = string(b)
            } else {
                rec[c] = v
            }
        }
        out = append(out, rec)
    }
    return out, rows.Err()
}