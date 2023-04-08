EXAMPLES := $(shell ls -d */ | sed -e 's,/,,g')

all: $(EXAMPLES)

$(EXAMPLES):
	./$@/run.sh
	make clean

clean:
	make -C ../Dockerfiles clean
	sudo pkill -9 neo

.PHONY: all $(EXAMPLES) clean
