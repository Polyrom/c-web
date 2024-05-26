build-server:
	clang -Wall server.c -o bin/server
run-server:
	./bin/server
build-playground:
	clang -Wall playground.c -o bin/playground
run-playground:
	./bin/playground
