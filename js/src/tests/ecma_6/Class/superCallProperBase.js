var test = `

class base1 {
    constructor() {
        this.base = 1;
    }
}

class base2 {
    constructor() {
        this.base = 2;
    }
}

class inst extends base1 {
    constructor() {
        super();
    }
}

assertEq(new inst().base, 1);

Object.setPrototypeOf(inst, base2);

assertEq(new inst().base, 2);

`;

if (classesEnabled())
    eval(test);

if (typeof reportCompare === 'function')
    reportCompare(0,0,"OK");
