# module téléinformation client
# rene-d 2020
.PHONY : build check checkall test docker

help:
	@echo lanceur de commandes pour dev

build:
	platformio run -e esp12 -t size -t buildfs

check:
	platformio check -e esp12

checkall:
	platformio run -e esp12 -t compiledb
	cppcheck --project=.pio/build/esp12/compile_commands.json --enable=all --xml 2>.pio/cppcheck.xml
	python3 /usr/local/bin/cppcheck-htmlreport --file .pio/cppcheck.xml --report-dir=.pio/cppcheck/
	@-open .pio/cppcheck/index.html

test:
	./runtest.sh
	@-open build/coverage.html
	@-open build/cppcheck/index.html

docker:
	docker buildx build -t test .
	docker run --rm -ti \
		-v $(PWD):/tic:ro \
		-v $(PWD)/build/build-docker:/build \
		-v $(PWD)/build/results-docker:/results \
		test \
		/tic/runtest.sh
	@-open build/results-docker/coverage.html
	@-open build/results-docker/cppcheck/index.html
