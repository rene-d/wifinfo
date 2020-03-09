# module téléinformation client
# rene-d 2020

# the default target
BOARD := esp12

.PHONY: help build upload check checkall test docker

help:
	@echo Developer shortcuts

build:
	platformio run -e $(BOARD)
	platformio run -e $(BOARD) -t buildfs

upload:
	platformio run -e $(BOARD) -t uploadfs
	platformio run -e $(BOARD) -t upload

check:
	platformio check -e $(BOARD)

checkall:
	platformio run -e $(BOARD) -t compiledb
	cppcheck --project=.pio/build/$(BOARD)/compile_commands.json --enable=all --xml 2>.pio/cppcheck.xml
	cppcheck-htmlreport --file .pio/cppcheck.xml --report-dir=.pio/cppcheck/
	@-which open && open .pio/cppcheck/index.html

test:
	./runtest.sh
	@-which open && open build/coverage.html
	@-which open && open build/cppcheck/index.html

docker:
	docker buildx build -t test --load .
	docker run --rm \
		-v $(PWD):/tic:ro \
		-v $(PWD)/build/build-docker:/build \
		-v $(PWD)/build/results-docker:/results \
		test \
		/tic/runtest.sh
	@-which open && open build/results-docker/coverage.html
	@-which open && open build/results-docker/cppcheck/index.html
