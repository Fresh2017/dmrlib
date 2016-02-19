.. _packetq:

packetq: DMR packet queue
=========================


Data types
----------

.. c:type:: dmr_packetq_entry

.. c:member:: dmr_packetq_entry.parsed
.. c:member:: dmr_packetq_entry.entries

.. c:type:: dmr_packetq

.. c:member:: dmr_packetq.head


API
---

.. c:function:: dmr_packetq * dmr_packetq_new()

   Allocates a new packet queue. Free with :c:func:`dmr_free`.

.. c:function:: int dmr_packetq_add(dmr_packetq *, dmr_parsed_packet *)

.. c:function:: int dmr_packetq_add_packet(dmr_packetq *, dmr_packet)

.. c:function:: int dmr_packetq_shift(dmr_packetq *, dmr_parsed_packet **)

.. c:function:: int dmr_packetq_shift_packet(dmr_packetq *, dmr_packet)

.. c:function:: int dmr_packetq_foreach(dmr_packetq *, dmr_parsed_packet_cb, void *)

.. c:function:: int dmr_packetq_foreach_packet(dmr_packetq *, dmr_packet_cb, void *)

