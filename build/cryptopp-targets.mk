libraries: $(LIBPREFIX)cryptopp.$(LIBSUFFIX)
LIBRARIES += $(LIBPREFIX)cryptopp.$(LIBSUFFIX)
$(LIBPREFIX)cryptopp.$(LIBSUFFIX):
	sh -c 'cd cryptopp; make static CXX="$(X86)"; cp libcryptopp.a ..;make clean; cd ..'
