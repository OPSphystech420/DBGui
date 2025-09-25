package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
)

func deleteHandler(w http.ResponseWriter, r *http.Request) {
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
		http.Error(w, "where parameter is required for delete", http.StatusBadRequest)
		return
	}

	query := fmt.Sprintf("DELETE FROM %s WHERE %s", table, where)
	log.Println("Executing query:", query)
	result, err := db.Exec(query)
	if err != nil {
		http.Error(w, "Delete error: "+err.Error(), http.StatusInternalServerError)
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
