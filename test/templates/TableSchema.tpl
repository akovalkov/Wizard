{%- set ignore_last_field_sep = false -%}
--
-- Table `{{ name }}`
--
CREATE TABLE IF NOT EXISTS `{{ name }}` (
## apply-template FieldSchema fields
) ENGINE=MyISAM  DEFAULT CHARSET=utf8;
-- --------------------------------------------------------
