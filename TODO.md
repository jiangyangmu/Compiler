JCC TODO list
===

EBNF Parser

+ support context in CODE.
    * what context should be provided in CODE node { ... } ?
        current node properties: SET_PROP(value), GET_PROP()
        direct child properties: SET_CHILD_PROP(i, value), GET_CHILD_PROP(i)
+ check possible empty production.
