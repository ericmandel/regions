/* eslint-disable dot-notation */

/*global ccall_varargs, getCFunc HEAPF64, HEAP32, assert, toC, EmterpreterAsync, Pointer_stringify, stackSave, stackAlloc, stackRestore, Module */

// C calling interface for varargs.
// Like ccall, but with a varargs array in the last argTypes argument
// containing 2 strings [nonrepArr, repArr] where:
//   nonrepArr: a string of non-repeating specifiers "n1n2,...nn"
//   repArr:    a string of repeating specifiers "r1r2...rn"
// valid specifiers are "s","i","u","d"
//
// example: pass a string, followed by a repeating series of double,int pairs:
// Module.ccall_varargs("miniprintf", "null", ["string", ["s", "di"]], ["%s %f %d %s %f %d %s\n", "foo", 1.234, 2, "foo1", 3.14, -100, "goo1"])
//
//
Module["ccall_varargs"] = function(ident, returnType, argTypes, args, opts) {
    var func = getCFunc(ident);
    var cArgs = [];
    var stack = 0;
    var ret, converter;
    var i, j;
    var maxDeclaredArgs, lastInputArg;
    var vSpecifiers=[[], []], vSpecifiersLen;
    var vArgs=[], vArgsLen;
    var vStack, vStackCur;
    var vToStack = function(item, offset, type) {
	var vrem;
	switch (type) {
        case "d":
            vrem = offset % 8;
            if (vrem !== 0) {
		offset += (8 - vrem);
	    }
	    HEAPF64.set([item], offset/8);
	    offset += 8;
            break;
	case 'f':
	case 'i':
	case 's':
	case 'u':
            vrem = offset % 4;
            if (vrem !== 0) {
		offset += (4 - vrem);
	    }
	    HEAP32.set([item], offset/4);
	    offset += 4;
            break;
	}
	return offset;
    };
    assert(returnType !== 'array', 'Return type should not be "array".');
    if (args) {
	lastInputArg = argTypes[argTypes.length-1];
	// look for varargs specifier in final type argument
	if (typeof lastInputArg === "object") {
	    // array of non-repeating args
	    if( lastInputArg[0] ){
		vSpecifiers[0] = lastInputArg[0].split("");
	    }
	    // array of repeating args
	    if( lastInputArg[1] ){
		vSpecifiers[1] = lastInputArg[1].split("");
	    }
            maxDeclaredArgs = argTypes.length-1;
	} else {
	    // no varargs, process all args as declared args
	    maxDeclaredArgs = args.length;
	}
	// process declared args
	for (i = 0; i < maxDeclaredArgs; i++) {
            converter = toC[argTypes[i]];
            if (converter) {
		if (stack === 0) {
		    stack = stackSave();
		}
		cArgs[i] = converter(args[i]);
            } else {
		cArgs[i] = args[i];
            }
	}
	// process varargs
	if (vSpecifiers[0].length || vSpecifiers[1].length) {
            // varargs go onto the stack
	    if (stack === 0) {
		stack = stackSave();
	    }
            // do string conversions before creating the varargs stack
	    for (i = maxDeclaredArgs, j = 0; i < args.length; i++, j++) {
		if (typeof args[i] === "string") {
		    vArgs[j] = toC["string"](args[i]);
		} else {
		    vArgs[j] = args[i];
		}
            }
	    // largest stack space we will need (if all varargs are doubles)
            vStack = stackAlloc(vArgs.length * 8);
            // current location in stack to which to write
            vStackCur = vStack;
	    // number of varargs processed thus far
	    i = 0;
            // write non-repeating varargs to stack space, with proper alignment
	    if (vSpecifiers[0].length) {
		vSpecifiersLen = vSpecifiers[0].length;
		for (j = 0; j < vSpecifiersLen; i++, j++) {
		    vStackCur = vToStack(vArgs[i], vStackCur,
					 vSpecifiers[0][j]);
		}
	    }
            // write repeating varargs to stack space, with proper alignment
	    if (vSpecifiers[1].length) {
		vSpecifiersLen = vSpecifiers[1].length;
		vArgsLen = vArgs.length - i;
		for (j = 0; j < vArgsLen; i++, j++) {
		    vStackCur = vToStack(vArgs[i], vStackCur,
					 vSpecifiers[1][j%vSpecifiersLen]);
		}
	    }
            // add final varargs argument: a pointer to varargs stack space
	    cArgs[maxDeclaredArgs] = vStack;
	}
    }
    ret = func.apply(null, cArgs);
    if ((!opts || !opts.async) && typeof EmterpreterAsync === 'object') {
	assert(!EmterpreterAsync.state,
	       'cannot start async op with normal JS calling ccall');
    }
    if (opts && opts.async) {
	assert(!returnType, 'async ccalls cannot return values');
    }
    if (returnType === 'string') {
	ret = Pointer_stringify(ret);
    }
    if (stack !== 0) {
	if (opts && opts.async) {
            EmterpreterAsync.asyncFinalizers.push(function() {
		stackRestore(stack);
            });
            return;
	}
	stackRestore(stack);
    }
    return ret;
};
