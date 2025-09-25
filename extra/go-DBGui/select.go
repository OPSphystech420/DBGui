package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
)

func selectHandler(w http.ResponseWriter, r *http.Request) {
	table := r.URL.Query().Get("table")
	if table == "" {
		http.Error(w, "table parameter is required", http.StatusBadRequest)
		return
	}

	columns := r.URL.Query().Get("columns")
	if columns == "" {
		columns = "*"
	}
	query := fmt.Sprintf("SELECT %s FROM %s", columns, table)

	if where := r.URL.Query().Get("where"); where != "" {
		query += " WHERE " + where
	}
	if groupby := r.URL.Query().Get("groupby"); groupby != "" {
		query += " GROUP BY " + groupby
	}
	if having := r.URL.Query().Get("having"); having != "" {
		query += " HAVING " + having
	}
	if orderby := r.URL.Query().Get("orderby"); orderby != "" {
		query += " ORDER BY " + orderby
	}
	if limit := r.URL.Query().Get("limit"); limit != "" {
		query += " LIMIT " + limit
	}
	if offset := r.URL.Query().Get("offset"); offset != "" {
		query += " OFFSET " + offset
	}

	log.Println("Executing query:", query)
	rows, err := db.Query(query)
	if err != nil {
		http.Error(w, "Query error: "+err.Error(), http.StatusInternalServerError)
		return
	}
	defer rows.Close()

	cols, err := rows.Columns()
	if err != nil {
		http.Error(w, "Error fetching columns: "+err.Error(), http.StatusInternalServerError)
		return
	}

	var results []map[string]interface{}
	for rows.Next() {
		columns := make([]interface{}, len(cols))
		columnPointers := make([]interface{}, len(cols))
		for i := range columns {
			columnPointers[i] = &columns[i]
		}
		if err := rows.Scan(columnPointers...); err != nil {
			http.Error(w, "Row scan error: "+err.Error(), http.StatusInternalServerError)
			return
		}
		rowMap := make(map[string]interface{})
		for i, colName := range cols {
			val := columnPointers[i].(*interface{})
			rowMap[colName] = *val
		}
		results = append(results, rowMap)
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(results)
}
