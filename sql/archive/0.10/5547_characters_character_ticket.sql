ALTER  TABLE  `character_ticket`
    ADD  `ticket_lastchange` TIMESTAMP ON  UPDATE  CURRENT_TIMESTAMP  NOT  NULL  DEFAULT CURRENT_TIMESTAMP ;
