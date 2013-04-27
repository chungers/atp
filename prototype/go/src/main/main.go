package main

import (
	"github.com/hoisie/web"
	"borg"
	"log"
	"os"
)

var glogger *log.Logger
var server *web.Server

func exitServer() string {
	glogger.Println("Shutting down")
	defer server.Close()
	return "ok"
}

func shutdown(server *web.Server) func() string {
	return func() string {
		glogger.Println("Shutdown ", server)
		defer server.Close()
		return "stopped"
	}
}

func hello(val string) string {
	borg.Borg()
	return "Hello " + val
}

func hello2(ctx *web.Context, val string) string {
	path := ctx.Request.URL.Path
	glogger.Println("Got path = ", path)
	for k, v := range ctx.Params {
		glogger.Println(k, v)
	}
	return path + " " + val
}

func main() {
	f, err := os.Create("server.log")
	if err != nil {
		println(err.Error())
		return
	}

	glogger = log.New(f, "", log.Ldate|log.Ltime)

	server = web.NewServer()
	server.SetLogger(glogger)

	server.Get("/exit", exitServer)
	server.Get("/shutdown", shutdown(server))
	server.Get("/hello2/(.*)", hello2)
	server.Put("/hello2/(.*)", hello2)
	server.Get("/(.*)", hello)
	server.Run("0.0.0.0:9999")
}