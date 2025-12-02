const words = `aru abbemux aps
biliar bem braphus
cep crym ca
de durgh dweft
elliani erg ept ex
flam ferrinum fannel
gar geppina gelft
hoccet hal hex
illius intex ir
jep jantufex jowbus
knogh kipple kwn
leffimer lux lenta lasus
mu marph moop mewdle
nargoplex ner nan
o octrum ollix
pefta pax piom
que quenna quish
re rit romma relifi
si scunnia shew sluph
torph tem tallia twint
un umbria urqualle
vexularg vi vorren
wreng wook wumple
xa xento xeff
yoom yit yallimus
zon zettlier zushne`.split(/\s+/g);

let dst = "";
let sentenceLength = 0; // in words
for (let i=1000; i-->0; ) {
  let word = words[Math.floor(Math.random() * words.length)];
  if (!word) continue;
  if (!sentenceLength) {
    word = word[0].toUpperCase() + word.substring(1);
  }
  dst += word;
  sentenceLength++;
  if ((sentenceLength >= 3) && !(Math.floor(Math.random() * 4))) {
    dst += ". ";
    sentenceLength = 0;
  } else {
    dst += " ";
  }
}

console.log(dst);
