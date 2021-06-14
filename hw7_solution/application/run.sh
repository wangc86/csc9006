#!/bin/bash

echo "Starting name server"
${TAO_ROOT}/orbsvcs/Naming_Service/tao_cosnaming -o ns.ior &

sleep 2

echo "Starting event server"
./Service -ORBInitRef NameService=file://ns.ior -ORBsvcconf ec.conf &

sleep 3

echo "Starting consumer"
#./Consumer -ORBInitRef NameService=file://ns.ior > hw7log.out &
./Consumer -ORBInitRef NameService=file://ns.ior &

sleep 1

echo "Starting supplier"
./Supplier -ORBInitRef NameService=file://ns.ior
