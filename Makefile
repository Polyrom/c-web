build-server:
	clang -c http_server_client.c -I.
	clang -c http_server.c -I.
	clang http_server_client.o http_server.o -o bin/server
run-server:
	./bin/server
build-db:
	clang -Wall -o bin/db_adapter -lsqlite3 db.c
run-db:
	./bin/db_adapter
build-playground:
	clang -Wall playground.c -o bin/playground
run-playground:
	./bin/playground
