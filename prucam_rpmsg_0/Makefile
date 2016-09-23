all:
ifeq ($(RELEASE), 1)
	cd Release && $(MAKE) all
endif
ifeq ($(DEBUG), 1)
	cd Debug && $(MAKE) all
endif

clean:
ifeq ($(RELEASE), 1)
	cd Release && $(MAKE) clean
endif
ifeq ($(DEBUG), 1)
	cd Debug && $(MAKE) clean
endif
