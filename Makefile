SUBDIRS = liblali bgg-client

.PHONY: clean liblali deploy

all: bgg-client/tdj-crawler

bgg-client/tdj-crawler: liblali
	$(MAKE) -C bgg-client all

liblali:
	$(MAKE) -C liblali all

deploy: bgg-client/tdj-crawler
	mkdir bin
	mkdir bin/conf
	cp liblali/**/*.so bin/
	cp bgg-client/tdj-crawler bin/
	cp -r bgg-client/resources bin/
	cp -r bgg-client/templates bin/
	cp -r bgg-client/resources/tdj-crawler.conf bin/conf/

clean:
	for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done
