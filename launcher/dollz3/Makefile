DOL2ELF := dol2elf.exe

DOLLZ3_URL := https://wiibrew.org/w/images/e/ef/Dollz3.zip
ifeq ($(OS),Windows_NT)
DOLLZ3 := dollz3.exe
else
DOLLZ3 := dollz3
endif

all: $(DOL2ELF) dollz3.exe

$(DOL2ELF): dol2elf.c
	@echo compiling $< ...
	@$(CXX) -fpermissive -o $@ $<

dollz3.zip:
	@echo downloading $@ ...
	@curl -L $(DOLLZ3_URL) -o $@

dollz3.exe: dollz3.zip
	@echo extracting $@ ...
	@unzip -p $< dollz3/$(DOLLZ3) > $@
	@chmod +x $@

clean:
	@echo "clean ..."
	@rm -f $(DOL2ELF) dollz3.exe dollz3.zip

.PHONY: all clean
.DELETE_ON_ERROR:
