.PHONY: all clean

all:
	@$(MAKE) -C Atmosphere-libs/
	@$(MAKE) -C sys-botbase/

clean:
	@$(MAKE) clean -C Atmosphere-libs/
	@$(MAKE) clean -C sys-botbase/