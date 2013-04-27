package main

import (
	"borg"
	"github.com/hoisie/web"
	"log"
	"os"
)

func shutdown(server *web.Server) func() string {
	return func() string {
		server.Logger.Println("Shutdown server at port", server.Config)
		defer server.Close()
		return "stopped"
	}
}

func hello(ctx *web.Context, val string) string {
	borg.Borg()
	ctx.Server.Logger.Println("hello ==> " + val)
	return "Hello " + val
}

func hello2(ctx *web.Context, val string) string {
	path := ctx.Request.URL.Path
	ctx.Server.Logger.Println("Got path = ", path)
	for k, v := range ctx.Params {
		ctx.Server.Logger.Println(k, v)
	}
	return path + " " + val
}

func main() {
	f, err := os.Create("server.log")
	if err != nil {
		println(err.Error())
		return
	}

	server := web.NewServer()

	logger := log.New(f, "", log.Ldate|log.Ltime)
	server.SetLogger(logger)

	server.Get("/shutdown", shutdown(server))
	server.Get("/hello/(.*)", hello)
	server.Get("/hello2/(.*)", hello2)
	server.Put("/hello2/(.*)", hello2)
	server.Get("/(.*)", hello)
	server.Run("0.0.0.0:9999")
}
