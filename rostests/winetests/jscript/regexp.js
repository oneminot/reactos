/*
 * Copyright 2008 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


var m, re, b, i, obj;

re = /a+/;
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex);

m = re.exec(" aabaaa");
ok(re.lastIndex === 3, "re.lastIndex = " + re.lastIndex);
ok(m.index === 1, "m.index = " + m.index);
ok(m.input === " aabaaa", "m.input = " + m.input);
ok(m.length === 1, "m.length = " + m.length);
ok(m[0] === "aa", "m[0] = " + m[0]);

m = re.exec(" aabaaa");
ok(re.lastIndex === 3, "re.lastIndex = " + re.lastIndex);
ok(m.index === 1, "m.index = " + m.index);
ok(m.input === " aabaaa", "m.input = " + m.input);
ok(m.length === 1, "m.length = " + m.length);
ok(m[0] === "aa", "m[0] = " + m[0]);

re = /a+/g;
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex);

m = re.exec(" aabaaa");
ok(re.lastIndex === 3, "re.lastIndex = " + re.lastIndex);
ok(m.index === 1, "m.index = " + m.index);
ok(m.input === " aabaaa", "m.input = " + m.input);
ok(m.length === 1, "m.length = " + m.length);
ok(m[0] === "aa", "m[0] = " + m[0]);

m = re.exec(" aabaaa");
ok(re.lastIndex === 7, "re.lastIndex = " + re.lastIndex);
ok(m.index === 4, "m.index = " + m.index);
ok(m.input === " aabaaa", "m.input = " + m.input);
ok(m.length === 1, "m.length = " + m.length);
ok(m[0] === "aaa", "m[0] = " + m[0]);

m = re.exec(" aabaaa");
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex);
ok(m === null, "m is not null");

re.exec("               a");
ok(re.lastIndex === 16, "re.lastIndex = " + re.lastIndex);

m = re.exec(" a");
ok(m === null, "m is not null");
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex);

m = re.exec(" a");
ok(re.lastIndex === 2, "re.lastIndex = " + re.lastIndex);

m = re.exec();
ok(m === null, "m is not null");
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex);

m = /(a|b)+|(c)/.exec("aa");
ok(m[0] === "aa", "m[0] = " + m[0]);
ok(m[1] === "a", "m[1] = " + m[1]);
ok(m[2] === "", "m[2] = " + m[2]);

b = re.test("  a ");
ok(b === true, "re.test('  a ') returned " + b);
ok(re.lastIndex === 3, "re.lastIndex = " + re.lastIndex);

b = re.test(" a ");
ok(b === false, "re.test(' a ') returned " + b);
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex);

re = /\[([^\[]+)\]/g;
m = re.exec(" [test]  ");
ok(re.lastIndex === 7, "re.lastIndex = " + re.lastIndex);
ok(m.index === 1, "m.index = " + m.index);
ok(m.input === " [test]  ", "m.input = " + m.input);
ok(m.length === 2, "m.length = " + m.length);
ok(m[0] === "[test]", "m[0] = " + m[0]);
ok(m[1] === "test", "m[1] = " + m[1]);

b = /a*/.test();
ok(b === true, "/a*/.test() returned " + b);

m = "abcabc".match(re = /ca/);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 1, "m.length is not 1");
ok(m["0"] === "ca", "m[0] is not \"ca\"");
ok(m.constructor === Array, "unexpected m.constructor");
ok(re.lastIndex === 4, "re.lastIndex = " + re.lastIndex);

m = "abcabc".match(/ab/);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 1, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");

m = "abcabc".match(/ab/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "abcabc".match(/Ab/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m === null, "m is not null");

m = "abcabc".match(/Ab/gi);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "aaabcabc".match(/a+b/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "aaab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "aaa\\\\cabc".match(/\\/g);
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "\\", "m[0] is not \"\\\"");
ok(m["1"] === "\\", "m[1] is not \"\\\"");

m = "abcabc".match(new RegExp("ab"));
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 1, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");

m = "abcabc".match(new RegExp("ab","g"));
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "abcabc".match(new RegExp(/ab/g));
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "abcabc".match(new RegExp("ab","g", "test"));
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 2, "m.length is not 2");
ok(m["0"] === "ab", "m[0] is not \"ab\"");
ok(m["1"] === "ab", "m[1] is not \"ab\"");

m = "abcabcg".match("ab", "g");
ok(typeof(m) === "object", "typeof m is not object");
ok(m.length === 1, "m.length is not 1");
ok(m["0"] === "ab", "m[0] is not \"ab\"");

m = "abcabc".match();
ok(m === null, "m is not null");

r = "- [test] -".replace(re = /\[([^\[]+)\]/g, "success");
ok(r === "- success -", "r = " + r + " expected '- success -'");
ok(re.lastIndex === 8, "re.lastIndex = " + re.lastIndex);

r = "[test] [test]".replace(/\[([^\[]+)\]/g, "aa");
ok(r === "aa aa", "r = " + r + "aa aa");

r = "[test] [test]".replace(/\[([^\[]+)\]/, "aa");
ok(r === "aa [test]", "r = " + r + " expected 'aa [test]'");

r = "- [test] -".replace(/\[([^\[]+)\]/g);
ok(r === "- undefined -", "r = " + r + " expected '- undefined -'");

r = "- [test] -".replace(/\[([^\[]+)\]/g, true);
ok(r === "- true -", "r = " + r + " expected '- true -'");

r = "- [test] -".replace(/\[([^\[]+)\]/g, true, "test");
ok(r === "- true -", "r = " + r + " expected '- true -'");

var tmp = 0;

function replaceFunc1(m, off, str) {
    ok(arguments.length === 3, "arguments.length = " + arguments.length);

    switch(tmp) {
    case 0:
        ok(m === "[test1]", "m = " + m + " expected [test1]");
        ok(off === 0, "off = " + off + " expected 0");
        break;
    case 1:
        ok(m === "[test2]", "m = " + m + " expected [test2]");
        ok(off === 8, "off = " + off + " expected 8");
        break;
    default:
        ok(false, "unexpected call");
    }

    ok(str === "[test1] [test2]", "str = " + arguments[3]);
    return "r" + tmp++;
}

r = "[test1] [test2]".replace(/\[[^\[]+\]/g, replaceFunc1);
ok(r === "r0 r1", "r = " + r + " expected 'r0 r1'");

tmp = 0;

function replaceFunc2(m, subm, off, str) {
    ok(arguments.length === 4, "arguments.length = " + arguments.length);

    switch(tmp) {
    case 0:
        ok(subm === "test1", "subm = " + subm);
        ok(m === "[test1]", "m = " + m + " expected [test1]");
        ok(off === 0, "off = " + off + " expected 0");
        break;
    case 1:
        ok(subm === "test2", "subm = " + subm);
        ok(m === "[test2]", "m = " + m + " expected [test2]");
        ok(off === 8, "off = " + off + " expected 8");
        break;
    default:
        ok(false, "unexpected call");
    }

    ok(str === "[test1] [test2]", "str = " + arguments[3]);
    return "r" + tmp++;
}

r = "[test1] [test2]".replace(/\[([^\[]+)\]/g, replaceFunc2);
ok(r === "r0 r1", "r = '" + r + "' expected 'r0 r1'");

r = "$1,$2".replace(/(\$(\d))/g, "$$1-$1$2");
ok(r === "$1-$11,$1-$22", "r = '" + r + "' expected '$1-$11,$1-$22'");

r = "abc &1 123".replace(/(\&(\d))/g, "$&");
ok(r === "abc &1 123", "r = '" + r + "' expected 'abc &1 123'");

r = "abc &1 123".replace(/(\&(\d))/g, "$'");
ok(r === "abc  123 123", "r = '" + r + "' expected 'abc  123 123'");

r = "abc &1 123".replace(/(\&(\d))/g, "$`");
ok(r === "abc abc  123", "r = '" + r + "' expected 'abc abc  123'");

r = "abc &1 123".replace(/(\&(\d))/g, "$3");
ok(r === "abc $3 123", "r = '" + r + "' expected 'abc $3 123'");

r = "abc &1 123".replace(/(\&(\d))/g, "$");
ok(r === "abc $ 123", "r = '" + r + "' expected 'abc $ 123'");

r = "abc &1 123".replace(/(\&(\d))/g, "$a");
ok(r === "abc $a 123", "r = '" + r + "' expected 'abc $a 123'");

r = "abc &1 123".replace(/(\&(\d))/g, "$11");
ok(r === "abc &11 123", "r = '" + r + "' expected 'abc &11 123'");

r = "abc &1 123".replace(/(\&(\d))/g, "$0");
ok(r === "abc $0 123", "r = '" + r + "' expected 'abc $0 123'");

r = "1 2 3".replace("2", "$&");
ok(r === "1 $& 3", "r = '" + r + "' expected '1 $& 3'");

r = "1 2 3".replace("2", "$'");
ok(r === "1 $' 3", "r = '" + r + "' expected '1 $' 3'");

r = "1,,2,3".split(/,+/g);
ok(r.length === 3, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);
ok(r[2] === "3", "r[2] = " + r[2]);

r = "1,,2,3".split(/,+/);
ok(r.length === 3, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);
ok(r[2] === "3", "r[2] = " + r[2]);

r = "1,,2,".split(/,+/);
ok(r.length === 2, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);

re = /,+/;
r = "1,,2,".split(re);
ok(r.length === 2, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);
ok(re.lastIndex === 5, "re.lastIndex = " + re.lastIndex);

re = /,+/g;
r = "1,,2,".split(re);
ok(r.length === 2, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);
ok(re.lastIndex === 5, "re.lastIndex = " + re.lastIndex);

r = "1 12 \t3".split(re = /\s+/).join(";");
ok(r === "1;12;3", "r = " + r);
ok(re.lastIndex === 6, "re.lastIndex = " + re.lastIndex);

r = "123".split(re = /\s+/).join(";");
ok(r === "123", "r = " + r);
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex);

/* another standard violation */
r = "1 12 \t3".split(re = /(\s)+/g).join(";");
ok(r === "1;12;3", "r = " + r);
ok(re.lastIndex === 6, "re.lastIndex = " + re.lastIndex);

re = /,+/;
re.lastIndex = 4;
r = "1,,2,".split(re);
ok(r.length === 2, "r.length = " + r.length);
ok(r[0] === "1", "r[0] = " + r[0]);
ok(r[1] === "2", "r[1] = " + r[1]);
ok(re.lastIndex === 5, "re.lastIndex = " + re.lastIndex);

re = /abc[^d]/g;
ok(re.source === "abc[^d]", "re.source = '" + re.source + "', expected 'abc[^d]'");

re = /a\bc[^d]/g;
ok(re.source === "a\\bc[^d]", "re.source = '" + re.source + "', expected 'a\\bc[^d]'");

re = /abc/;
ok(re === RegExp(re), "re !== RegExp(re)");

re = RegExp("abc[^d]", "g");
ok(re.source === "abc[^d]", "re.source = '" + re.source + "', expected 'abc[^d]'");

re = /abc/;
ok(re === RegExp(re, undefined), "re !== RegExp(re, undefined)");

re = /abc/;
ok(re === RegExp(re, undefined, 1), "re !== RegExp(re, undefined, 1)");

re = /a/g;
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex + " expected 0");

m = re.exec(" a   ");
ok(re.lastIndex === 2, "re.lastIndex = " + re.lastIndex + " expected 2");
ok(m.index === 1, "m.index = " + m.index + " expected 1");

m = re.exec(" a   ");
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex + " expected 0");
ok(m === null, "m = " + m + " expected null");

re.lastIndex = 2;
m = re.exec(" a a ");
ok(re.lastIndex === 4, "re.lastIndex = " + re.lastIndex + " expected 4");
ok(m.index === 3, "m.index = " + m.index + " expected 3");

re.lastIndex = "2";
ok(re.lastIndex === "2", "re.lastIndex = " + re.lastIndex + " expected '2'");
m = re.exec(" a a ");
ok(re.lastIndex === 4, "re.lastIndex = " + re.lastIndex + " expected 4");
ok(m.index === 3, "m.index = " + m.index + " expected 3");

var li = 0;
var obj = new Object();
obj.valueOf = function() { return li; };

re.lastIndex = obj;
ok(re.lastIndex === obj, "re.lastIndex = " + re.lastIndex + " expected obj");
li = 2;
m = re.exec(" a a ");
ok(re.lastIndex === 2, "re.lastIndex = " + re.lastIndex + " expected 2");
ok(m.index === 1, "m.index = " + m.index + " expected 1");

re.lastIndex = 3;
re.lastIndex = "test";
ok(re.lastIndex === "test", "re.lastIndex = " + re.lastIndex + " expected 'test'");
m = re.exec(" a a ");
ok(re.lastIndex === 2 || re.lastIndex === 0, "re.lastIndex = " + re.lastIndex + " expected 2 or 0");
if(re.lastIndex != 0)
    ok(m.index === 1, "m.index = " + m.index + " expected 1");
else
    ok(m === null, "m = " + m + " expected null");

re.lastIndex = 0;
re.lastIndex = 3.9;
ok(re.lastIndex === 3.9, "re.lastIndex = " + re.lastIndex + " expected 3.9");
m = re.exec(" a a ");
ok(re.lastIndex === 4, "re.lastIndex = " + re.lastIndex + " expected 4");
ok(m.index === 3, "m.index = " + m.index + " expected 3");

obj.valueOf = function() { throw 0; }
re.lastIndex = obj;
ok(re.lastIndex === obj, "unexpected re.lastIndex");
m = re.exec(" a a ");
ok(re.lastIndex === 2, "re.lastIndex = " + re.lastIndex + " expected 2");
ok(m.index === 1, "m.index = " + m.index + " expected 1");

re.lastIndex = -3;
ok(re.lastIndex === -3, "re.lastIndex = " + re.lastIndex + " expected -3");
m = re.exec(" a a ");
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex + " expected 0");
ok(m === null, "m = " + m + " expected null");

re.lastIndex = -1;
ok(re.lastIndex === -1, "re.lastIndex = " + re.lastIndex + " expected -1");
m = re.exec("  ");
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex + " expected 0");
ok(m === null, "m = " + m + " expected null");

re = /aa/g;
i = 'baacd'.search(re);
ok(i === 1, "'baacd'.search(re) = " + i);
ok(re.lastIndex === 3, "re.lastIndex = " + re.lastIndex);

re.lastIndex = 2;
i = 'baacdaa'.search(re);
ok(i === 1, "'baacd'.search(re) = " + i);
ok(re.lastIndex === 3, "re.lastIndex = " + re.lastIndex);

re = /aa/;
i = 'baacd'.search(re);
ok(i === 1, "'baacd'.search(re) = " + i);
ok(re.lastIndex === 3, "re.lastIndex = " + re.lastIndex);

re.lastIndex = 2;
i = 'baacdaa'.search(re);
ok(i === 1, "'baacd'.search(re) = " + i);
ok(re.lastIndex === 3, "re.lastIndex = " + re.lastIndex);

re = /d/g;
re.lastIndex = 1;
i = 'abc'.search(re);
ok(i === -1, "'abc'.search(/d/g) = " + i);
ok(re.lastIndex === 0, "re.lastIndex = " + re.lastIndex);

i = 'abcdde'.search(/[df]/);
ok(i === 3, "'abc'.search(/[df]/) = " + i);

i = 'abcdde'.search(/[df]/, "a");
ok(i === 3, "'abc'.search(/[df]/) = " + i);

i = 'abcdde'.search("[df]");
ok(i === 3, "'abc'.search(/d*/) = " + i);

obj = {
    toString: function() { return "abc"; }
};
i = String.prototype.search.call(obj, "b");
ok(i === 1, "String.prototype.seatch.apply(obj, 'b') = " + i);

i = " undefined ".search();
ok(i === null, "' undefined '.search() = " + i);

reportSuccess();
