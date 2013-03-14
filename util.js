var util = require('util');

var obj = { name: 'nate' };
obj.inspect = function(depth) {
      return '{' + this.name + '}';
};

console.log(util.inspect(obj));
