SUBDIRS = liblali bgg-client

.PHONY: clean liblali

all: liblali tdj-crawler

tdj-crawler: liblali
	$(MAKE) -C bgg-client all

liblali:
	$(MAKE) -C liblali all

clean:
	for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done
