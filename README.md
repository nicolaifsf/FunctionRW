# FunctionRW
PANDA plugin that records data on function read and writes

Installing:

1) Install panda-re/panda: https://github.com/panda-re/panda (for mac, check the file called install_mac.sh makes life much easier) 

2) In panda/panda/plugins/ add folder from this repository "funcRW": `$ mv ./funcRW $PANDAHOME/panda/plugins` 

3) Change panda/panda/plugins/config.panda add "funcRW" to the list `$ vi $PANDAHOME/panda/plugins` 

4) Navigate to panda/build and run `make`
