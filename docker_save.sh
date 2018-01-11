#!/bin/bash

docker login --username alrikai 
docker tag fishlabeler:latest alrikai/nrt                                                                                                                                                                   
docker push alrikai/nrt 
