// Run this script on a local JS installation
// Or
// Run this script online at a place like https://playcode.io

// Replace the input string with the AOB that you want to convert
const input = "E8 ?? ?? ?? ?? 48 8B 4C 24 ?? 8B FD 48 85 C9";

const regex = /(.)(.) /gm;
let output = input.replace(regex, "$1 $2/");
output = output.substr(0, output.length - 1) + " " + output.substr(output.length - 1, output.length)
console.log(output)