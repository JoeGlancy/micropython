Buttons
*******

.. py::module:: microbit

There are two buttons on the board, called ``button_a`` and ``button_b``.

Attributes
==========


.. py:attribute:: button_a

    A ``Button`` instance (see below) representing the left button.


.. py:attribute:: button_b

    Represents the right button.


Classes
=======

.. py:class:: Button()

    Represents a button.

    .. note::
        This class is not actually available to the user, it is only used by
        the two button instances, which are provided already initialized.


    .. py:method:: is_pressed()

        Returns ``True`` if the specified button ``button`` is pressed, and
        ``False`` otherwise.

    .. py:method:: is_long_pressed()

        Returns ``True`` if the specified button ``button`` is long pressed (ie:
        is being held down for an extended period of time), and ``False``
        otherwise.

    .. py:method:: was_pressed()

        Returns ``True`` or ``False`` to indicate if the button was pressed
        since the device started or the last time this method was called.

    .. py:method:: get_presses()

        Returns the running total of button presses, and resets this total
        to zero before returning.

    .. py:method:: was_clicked()

        Returns ``True`` or ``False`` to indicate if the button was clicked
        since the device started or the last time this method was called.

    .. py:method:: was_double_clicked()

        Returns ``True`` or ``False`` to indicate if the button was double
        clicked (two clicks in quick succession) since the device started or the
        last time this method was called.

    .. py:method:: was_long_clicked()

        Returns ``True`` or ``False`` to indicate if the button was long clicked
        (ie: clicked, but held down in between for an extended preiod of time)
        since the device started or the last time this method was called.



Example
=======

.. code::

    import microbit

    while True:
        if microbit.button_a.is_pressed() and microbit.button_b.is_pressed():
            microbit.display.scroll("AB")
            break
        elif microbit.button_a.is_pressed():
            microbit.display.scroll("A")
        elif microbit.button_b.is_pressed():
            microbit.display.scroll("B")
        microbit.sleep(100)
