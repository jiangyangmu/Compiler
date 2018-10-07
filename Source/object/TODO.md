Object System
===

"Lifecycle management."

+ The elements of Object System

    object = { storage duration, location }

    storage duration: how long to keep objects?
        static - storage is reserved, exists and retains its last-stored value
                 throughout the execution of the entire program. initialized
                 only once, prior to program startup.
            internal linkage OR external linkage OR 'static'
        automatic - storage is reserved for a new instance of such an object
                    on each normal entry into the block in which it is
                    declared, or on a jump from outside the block to a label in
                    the block or in an enclosed block. And is no longer
                    guaranteed to be reserved when execution of the block ends
                    in any way.
                    initialization (if exists) is performed on each normal
                    entry, but not if the block is entered by a jump to a label.
            no linkage AND no 'static'

    * implementation-defined limits: restrict the significance of an external name (an identifier that has external linkage) to six characters and may ignore distinctions of alphabetical case for such names
    * undefined behavior: The implementation shall treat at least the first 31 characters of an internal name (a macro name or an identifier that does not have external linkage) as significant. Any identifiers that differ in a significant character are different identifiers. If two identifiers differ in a non-significant character, the behavior is undefined.
    * indeterminate: The value of a pointer that referred to an object with automatic storage duration that is no longer guaranteed to be reserved is indeterminate.
