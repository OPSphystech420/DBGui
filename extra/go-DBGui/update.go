package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"strings"
)

func updateHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Only POST method is allowed", http.StatusMethodNotAllowed)
		return
	}
	if err := r.ParseForm(); err != nil {
		http.Error(w, "Error parsing form: "+err.Error(), http.StatusBadRequest)
		return
	}

	table := r.FormValue("table")
	if table == "" {
		http.Error(w, "table parameter is required", http.StatusBadRequest)
		return
	}
	where := r.FormValue("where")
	if where == "" {
		http.Error(w, "where parameter is required for update", http.StatusBadRequest)
		return
	}

	var setClauses []string
	var values []interface{}
	for key, vals := range r.Form {
		if key == "table" || key == "where" {
			continue
		}
		setClauses = append(setClauses, fmt.Sprintf("%s = ?", key))
		values = append(values, vals[0])
	}
	if len(setClauses) == 0 {
		http.Error(w, "No data provided for update", http.StatusBadRequest)
		return
	}

	query := fmt.Sprintf("UPDATE %s SET %s WHERE %s", table, strings.Join(setClauses, ", "), where)
	log.Println("Executing query:", query, "with values:", values)
	result, err := db.Exec(query, values...)
	if err != nil {
		http.Error(w, "Update error: "+err.Error(), http.StatusInternalServerError)
		return
	}
	rowsAffected, err := result.RowsAffected()
	if err != nil {
		http.Error(w, "Error fetching rows affected: "+err.Error(), http.StatusInternalServerError)
		return
	}

	resp := map[string]interface{}{
		"rows_affected": rowsAffected,
	}
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(resp)
}
