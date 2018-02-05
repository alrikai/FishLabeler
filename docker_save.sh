#!/bin/bash
docker build -f ./Dockerfile -t fishlabeler .
docker login --username alrikai 
docker tag fishlabeler:latest alrikai/nrt                                                                                                                                                                   
docker push alrikai/nrt 
