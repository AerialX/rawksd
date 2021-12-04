all: launcher rawksd riifs

launcher:
	@$(MAKE) --no-print-directory -C launcher z

rawksd:
	@$(MAKE) --no-print-directory -C rawksd z

riifs:
	@$(MAKE) --no-print-directory -C riifs/server-c

clean:
	@$(MAKE) -C rawksd clean
	@$(MAKE) -C launcher clean
	@$(MAKE) -C megamodule clean
	@$(MAKE) -C filemodule clean
	@$(MAKE) -C dipmodule clean
	@$(MAKE) -C libios clean
	@$(MAKE) -C riifs/server-c clean

.PHONY: clean all launcher rawksd riifs
.NOTPARALLEL:
