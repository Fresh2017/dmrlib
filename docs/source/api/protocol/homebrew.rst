.. _homebrew:

Homebrew IP Site Connect
========================


Data types
----------

.. c:type:: dmr_homebrew_state

.. c:var:: DMR_HOMEBREW_AUTH_NONE
.. c:var:: DMR_HOMEBREW_AUTH_INIT
.. c:var:: DMR_HOMEBREW_AUTH_CONFIG
.. c:var:: DMR_HOMEBREW_AUTH_DONE
.. c:var:: DMR_HOMEBREW_AUTH_FAILED

.. c:type:: dmr_homebrew
.. c:member:: struct         dmr_homebrew.config
.. c:member:: char          *dmr_homebrew.config.call
.. c:member:: dmr_id         dmr_homebrew.config.repeater_id
.. c:member:: uint16_t       dmr_homebrew.config.rx_freq
.. c:member:: uint16_t       dmr_homebrew.config.tx_freq
.. c:member:: uint8_t        dmr_homebrew.config.tx_power
.. c:member:: dmr_color_code dmr_homebrew.config.color_code
.. c:member:: double         dmr_homebrew.config.latitude
.. c:member:: double         dmr_homebrew.config.longitude
.. c:member:: uint16_t       dmr_homebrew.config.altitude
.. c:member:: char          *dmr_homebrew.config.location
.. c:member:: char          *dmr_homebrew.config.description
.. c:member:: char          *dmr_homebrew.config.url
.. c:member:: char          *dmr_homebrew.config.software_id
.. c:member:: char          *dmr_homebrew.config.package_id
.. c:member:: char *dmr_homebrew.id

   Protocol identificaiton string.

.. c:member:: void           *dmr_homebrew.sock
.. c:member:: uint8_t         dmr_homebrew.peer_ip[16]
.. c:member:: uint16_t        dmr_homebrew.peer_port
.. c:member:: uint8_t         dmr_homebrew.bind_ip[16]
.. c:member:: uint16_t        dmr_homebrew.bind_port
.. c:member:: uint8_t         dmr_homebrew.state
.. c:member:: char           *dmr_homebrew.secret

   Authentication secret set by :c:func:`dmr_homebrew_auth`.

.. c:member:: uint8_t         dmr_homebrew.nonce[8]

   Nonce sent by repeater.

.. c:member:: dmr_packetq    *dmr_homebrew.rxq
.. c:member:: dmr_packetq    *dmr_homebrew.txq
.. c:member:: dmr_rawq       *dmr_homebrew.rrq
.. c:member:: dmr_rawq       *dmr_homebrew.trq
.. c:member:: struct timeval  dmr_homebrew.last_ping
.. c:member:: struct timeval  dmr_homebrew.last_pong       /* last pong received */


API
---

.. c:macro:: DMR_HOMEBREW_PORT

   Default Homebrew protocol UDP/IP port.

.. c:function:: dmr_homebrew * dmr_homebrew_new(dmr_id repeater_id, uint8_t peer_ip[16], uint16_t peer_port, uint8_t bind_ip[16], uint16_t bind_port)

   Setup a new Homebrew instance.

   This function sets up the internal structure as well as the communication
   sockets, the caller must provide data to the internal config struct before
   :c:func:`dmr_homebrew_auth` is called.

.. c:function:: int dmr_homebrew_auth(dmr_homebrew *homebrew, char *secret)

   Initiate authentication with the repeater.

.. c:function:: int dmr_homebrew_close(dmr_homebrew *homebrew)

   Close the link with the repeater.

.. c:function:: int dmr_homebrew_read(dmr_homebrew *homebrew, dmr_parsed_packet **parsed_out)

   Read a packet from the repeater.

   This also processes communications with the Homebrew repeater, if the
   received frame does not contain a DMR packet, the function will set the
   destination packet pointer to `NULL`.

.. c:function:: int dmr_homebrew_send(dmr_homebrew *homebrew, dmr_parsed_packet *parsed)

   Send a DMR frame to the repeater.

.. c:function:: int dmr_homebrew_send_buf(dmr_homebrew *homebrew, uint8_t *buf, size_t len)

   Send a raw buffer to the repeater.

.. c:function:: int dmr_homebrew_send_raw(dmr_homebrew *homebrew, dmr_raw *raw)

   Send a raw packet to the repeater.

.. c:function:: int dmr_homebrew_parse_dmrd(dmr_homebrew *homebrew, dmr_raw *raw, dmr_parsed_packet **parsed_out)

   Parse a Homebrew protocol DMR data frame.

.. c:var:: dmr_protocol dmr_homebrew_protocol

   Protocol specification.
