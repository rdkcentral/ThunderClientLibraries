#!/bin/bash

#check and install required packages
function checkandInstallPackage(){
	dpkg -s "$1" &> /dev/null
	if [ $? -eq 0 ]; then
    		echo "Package $1 is installed!"
	else
    		echo "Installing package $1"
		sudo apt-get install -y $1
	fi
	return $?
}

while getopts ":c" arg; do
  case $arg in
     c)
	echo "set coverage"
	coverage=1
	;;
     \?)
      	echo "Invalid option: -$OPTARG" >&2
      	echo "Usage: [-c coverage]" >&2
	exit 1
	;;
  esac
done

#Install required packages
checkandInstallPackage libgtest-dev
checkandInstallPackage libgmock-dev
checkandInstallPackage googletest
checkandInstallPackage lcov
checkandInstallPackage jq

#Delete previous test report
rm TestReport.json

#Run the L1 tests
cd L1Tests/
if [ "$coverage" == 1 ]; then
	./run.sh -c
else
	./run.sh
fi

cd ../


#Run the L2 tests
cd L2Tests/
if [ "$coverage" == 1 ]; then
	./run.sh -c
else
        ./run.sh
fi

cd ../

#Generate combined report of L1 and L2 test results
find . -name test_detail\*.json | xargs cat |  jq -s '{test_cases_results: {tests: map(.tests) | add,failures: map(.failures) | add,disabled: map(.disabled) | add,errors: map(.errors) | add,time: ((map(.time | rtrimstr("s") | tonumber) | add) | tostring + "s"),name: .[0].name,testsuites: map(.testsuites[])}}' > TestReport.json
