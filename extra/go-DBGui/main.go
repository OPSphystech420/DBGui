package main

import (
	"database/sql"
	"log"
	"net/http"

	_ "github.com/go-sql-driver/mysql"
)

var db *sql.DB

func main() {
	var err error

	dsn := "root:root@tcp(127.0.0.1:3306)/dbgui"
	db, err = sql.Open("mysql", dsn)
	if err != nil {
		log.Fatal("Error opening database:", err)
	}
	if err = db.Ping(); err != nil {
		log.Fatal("Error connecting to database:", err)
	}

	http.HandleFunc("/select", selectHandler)
	http.HandleFunc("/insert", insertHandler)
	http.HandleFunc("/update", updateHandler)
	http.HandleFunc("/delete", deleteHandler)
	http.HandleFunc("/call", callHandler)

	port := ":8080"
	log.Printf("Server running on %s\n", port)
	if err := http.ListenAndServe(port, nil); err != nil {
		log.Fatal("ListenAndServe error:", err)
	}
}
