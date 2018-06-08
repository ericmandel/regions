# where to find libraries
ASTROEM	=	../../astroem

XSRCS =  	xregcnts.c
XOBJS = 	$(XSRCS:%.c=%.o)

OBJS =		../jsfitsio.o ../healpix.o

LIBS =		../libregions.a -L$(ASTROEM)/lib -lcfitsio -lwcs -lz -lutil -lem

# debugging with symbols
# EMFLAGS	= 	-O0 -g4 --memory-init-file 0  -s ASSERTIONS=1
# production
EMFLAGS	= 	-O3 --memory-init-file 0

# cfitsio requires memory growth
EMOPTS =	-s TOTAL_MEMORY=67108864 -s ALLOW_MEMORY_GROWTH=1 \
		--no-heap-copy 

# wasm support:
WASMOPTS =	-s WASM=1

EMEXPORTS =	-s EXPORTED_FUNCTIONS="['_imannulusi', '_imboxi', '_imcirclei', '_imellipsei', '_imfieldi', '_imlinei', '_impiei', '_imqtpiei', '_impointi', '_impandai', '_imnannulusi', '_imnboxi', '_imnellipsei', '_imnpiei', '_impolygoni', '_imvannulusi', '_imvboxi', '_imvellipsei', '_imvpiei', '_imvpointi', '_imannulus', '_imbox', '_imcircle', '_imellipse', '_imfield', '_imline', '_impie', '_imqtpie', '_impoint', '_impanda', '_imnannulus', '_imnbox', '_imnellipse', '_imnpie', '_impolygon', '_imvannulus', '_imvbox', '_imvellipse', '_imvpie', '_imvpoint', '_regcnts']" -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap']"

# extras to build emscripten runtime
EMEXTRAS =	--pre-js pre.js --post-js ccall_varargs.js --preload-file snr.fits

# combine into EMCC command
EMCC =		emcc $(EMFLAGS) $(EMOPTS) $(EMEXPORTS) $(EMEXTRAS)

all:		em

xregcnts.o:	xregcnts.c
		$(EMCC) -I../util -c -o $@ $+

em:		FORCE
		@(cd ..; \
		  emconfigure ./configure --with-cfitsio=../astroem; \
		  emmake make clean emlib; \
		  cd em; \
		  make xregcnts.o; \
		  cp -p ../snr.fits .; \
		  echo "generating raw regcnts.js ..."; \
		  $(EMCC) $(XOBJS) $(OBJS) $(LIBS) -o regcnts.js; \
		  echo "adding shell wrapper to generate regcnts.js ..."; \
		  rm -f full_regcnts.js && touch full_regcnts.js; \
		  cat shell-pre.js >> full_regcnts.js; \
		  cat regcnts.js >> full_regcnts.js; \
		  cat shell-post.js >> full_regcnts.js; \
		  mv full_regcnts.js regcnts.js; \
		  rm -f snr.fits; \
		  echo "regcnts.js: load regcnts.html to run regcnts";)

miniprintf:	FORCE
		emcc -O0 -g4 -s ASSERTIONS=1 -s EXPORTED_FUNCTIONS="['_miniprintf']" miniprintf.c -o miniprintf.js --pre-js pre.js --post-js post.js

clean:		FORCE
		@($(RM) *{.a,.so,.so.?,.so.?.?,.o,.dylib} \
		  *.exe core core.[0-9]* errs *pure* .nfs* \
		  foo* *~ *.log \#* TAGS *.E a.out* errors \
		  gmon.out *.pg *.bak regtest.{pr,dy,co} \
	 	  regcnts.{js,js.map,wasm*,wast,data,bc} \
		  miniprintf.{js,js.map,wasm*,wast,data,bc})

FORCE:
