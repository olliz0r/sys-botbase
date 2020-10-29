.PHONY: all clean

all:
	@$(MAKE) -C sys-botbase/

clean:
	@$(MAKE) clean -C sys-botbase/