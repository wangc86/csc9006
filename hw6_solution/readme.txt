---------------------------
Answer for Assignment 1
---------------------------
either
(1) git switch -c hw6
or
(1) git branch hw6
(2) git switch hw6
or
(1) git branch hw6
(2) git checkout hw6


---------------------------
Answer for Assignment 2
---------------------------
TAO_EC_Supplier_Filter or
TAO_EC_Per_Supplier_Filter.
Here is how I did the tracing:
TAO_EC_ProxyPushConsumer_Guard is defined
in file EC_ProxyConsumer.h
Its constructor is defined in file
EC_ProxyConsumer.cpp
in it the 'filter' variable is assigned to
the return value of proxy_->filter_i ();
proxy_ is assigned to the last argument
of the constructor function, which is
TAO_EC_Default_ProxyPushConsumer, where
the 'filter_i ()' function is defined in
its parent class, i.e., TAO_EC_ProxyPushConsumer.
at line 131 in file EC_ProxyConsumer.h,
along with the comment at the line above it,
we can see that this function will return a
pointer to an object of TAO_EC_Supplier_Filter,
which will be either TAO_EC_Supplier_Filter (if
we configure the event channel to use the
trivial supplier filter)
or TAO_EC_Per_Supplier_Filter (if we configure
the event channel to use the per supplier
filter). Either way, the object is created by
the create function of the corresponding
supplier filter builder.


---------------------------
Answer for Assignment 3
---------------------------
COPY_ON_READ, LIST, and MT
Here is how I did the tracing:
At line 13 of file EC_Default_Factory.inl,
supplier_collection_ is assigned to
TAO_EC_DEFAULT_SUPPLIER_COLLECTION,
which is defined in file EC_Defaults.h
with value equal to 0x001.
According to lines 831-835 in file
EC_Default_Factory.cpp we see the answer.


---------------------------
Answer for Assignment 4
---------------------------
First we build a filter for this event consumer
(lines 95-96); then we call adopt_child ()
to point the parent_ pointer of child_ to
to this object, which is of class
TAO_EC_Default_ProxyPushSupplier and inherits
TAO_EC_Filter.
This parent_ pointer is used for the
event channel to push an event
to this consumer.


