package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"strings"
)

func insertHandler(w http.ResponseWriter, r *http.Request) {
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

	var columns []string
	var placeholders []string
	var values []interface{}
	for key, vals := range r.Form {
		if key == "table" {
			continue
		}
		columns = append(columns, key)
		placeholders = append(placeholders, "?")
		values = append(values, vals[0])
	}
	if len(columns) == 0 {
		http.Error(w, "No data provided for insert", http.StatusBadRequest)
		return
	}

	query := fmt.Sprintf("INSERT INTO %s (%s) VALUES (%s)", table, strings.Join(columns, ", "), strings.Join(placeholders, ", "))
	log.Println("Executing query:", query, "with values:", values)
	result, err := db.Exec(query, values...)
	if err != nil {
		http.Error(w, "Insert error: "+err.Error(), http.StatusInternalServerError)
		return
	}
	lastID, err := result.LastInsertId()
	if err != nil {
		http.Error(w, "Error fetching last insert id: "+err.Error(), http.StatusInternalServerError)
		return
	}

	resp := map[string]interface{}{
		"last_insert_id": lastID,
	}
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(resp)
}
