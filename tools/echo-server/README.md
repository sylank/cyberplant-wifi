https://hub.docker.com/r/ealen/echo-server

curl -X POST http://localhost:3000/api/sensor-data -H 'Content-Type: application/json' -d '{"login":"my_login","password":"my_password"}' | jq .