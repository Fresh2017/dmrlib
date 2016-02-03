#!/bin/bash

wget -O users.json.new 'http://www.dmr-marc.net/cgi-bin/trbo-database/datadump.cgi?format=json&table=users' && mv users.json.new users.json
