!/bin/bash

for i in {1..100}; do
  (sleep 1 && printf "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n") | nc localhost 4221 &
done
wait