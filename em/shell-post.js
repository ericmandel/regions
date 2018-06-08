//
// regprog_em.c defines the calling sequence prefix to be "Astroem." for all of
// the region routines. Thus, we need to return an object called Astroem
// containing the wrapped calls.
//
// The imv- routines use ccall_varargs(), which we are working to add to
// Emscripten (10/19/2017).
//
Astroem = {
imannulusi: Module.cwrap('imannulusi', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
imboxi: Module.cwrap('imboxi', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
imcirclei: Module.cwrap('imcirclei', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
imellipsei: Module.cwrap('imellipsei', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
imfieldi: Module.cwrap('imfieldi', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number']),
imlinei: Module.cwrap('imlinei', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
impiei: Module.cwrap('impiei', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
imqtpiei: Module.cwrap('imqtpiei', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
impointi: Module.cwrap('impointi', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
impandai: Module.cwrap('impandai', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
imnannulusi: Module.cwrap('imnannulusi', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
imnboxi: Module.cwrap('imnboxi', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
imnellipsei: Module.cwrap('imnellipsei', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
imnpiei: Module.cwrap('imnpiei', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
impolygoni: function(){
    return Module.ccall_varargs('impolygoni', 'number',
				[ 'number', 'number', 'number', 'number', 'number', 'number', 'number', ['','d'] ],
				Array.prototype.slice.call(arguments));
},
imvannulusi: function(){
    return Module.ccall_varargs('imvannulusi', 'number',
				[ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', ['','d'] ],
			       Array.prototype.slice.call(arguments));
},
imvboxi: function(){
    return Module.ccall_varargs('imvboxi', 'number',
				[ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', ['','d'] ],
			       Array.prototype.slice.call(arguments));
},
imvellipsei: function(){
    return Module.ccall_varargs('imvellipsei', 'number',
				[ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', ['','d'] ],
			       Array.prototype.slice.call(arguments));
},
imvpiei: function(){
    return Module.ccall_varargs('imvpiei', 'number',
				[ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', ['','d'] ],
			       Array.prototype.slice.call(arguments));
},
imvpointi: function(){
    return Module.ccall_varargs('imvpointi', 'number',
				[ 'number', 'number', 'number', 'number', 'number', 'number', 'number', ['','d'] ],
			       Array.prototype.slice.call(arguments));
},
imannulus: Module.cwrap('imannulus', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
imbox: Module.cwrap('imbox', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
imcircle: Module.cwrap('imcircle', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
imellipse: Module.cwrap('imellipse', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
imfield: Module.cwrap('imfield', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number']),
imline: Module.cwrap('imline', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
impie: Module.cwrap('impie', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
imqtpie: Module.cwrap('imqtpie', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
impoint: Module.cwrap('impoint', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
impanda: Module.cwrap('impanda', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
imnannulus: Module.cwrap('imnannulus', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']), 
imnbox: Module.cwrap('imnbox', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
imnellipse: Module.cwrap('imnellipse', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']),
imnpie: Module.cwrap('imnpie', 'number', [ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number']), 
impolygon: function(){
    return Module.ccall_varargs('impolygon', 'number',
				[ 'number', 'number', 'number', 'number', 'number', 'number', 'number', ['','d'] ],
				Array.prototype.slice.call(arguments));
},
imvannulus: function(){
    return Module.ccall_varargs('imvannulus', 'number',
				[ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', ['','d'] ],
				Array.prototype.slice.call(arguments));
},
imvbox: function(){
    return Module.ccall_varargs('imvbox', 'number',
				[ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', ['','d'] ],
				Array.prototype.slice.call(arguments));
},
imvellipse: function(){
    return Module.ccall_varargs('imvellipse', 'number',
				[ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', ['','d'] ],
				Array.prototype.slice.call(arguments));
},
imvpie: function(){
    return Module.ccall_varargs('imvpie', 'number',
				[ 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', 'number', ['','d'] ],
				Array.prototype.slice.call(arguments));
},
imvpoint: function(){
    return Module.ccall_varargs('imvpoint', 'number',
				[ 'number', 'number', 'number', 'number', 'number', 'number', 'number', ['','d'] ],
				Array.prototype.slice.call(arguments));
},
regcnts: Module.cwrap('regcnts', 'string', ['string', 'string', 'string', 'string'])
}
