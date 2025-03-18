{%- set ignore_last_field_sep = true -%}
--
-- Table `{{ name }}`
--
CREATE TABLE IF NOT EXISTS `{{ name }}` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
## apply-template FieldSchema fields
  `date_created` datetime NOT NULL,
  `date_updated` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;
-- --------------------------------------------------------

