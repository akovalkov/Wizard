  `{{ name }}` 
{#- The field type, one of:
    string, text, integer, object, money, discount, 
    enum, boolean, date, time, datetime or timestamp -#}        
{%- if type == "string" %} varchar({{ default(length, 255) }})
{%- else if type == "text" %} text
{%- else if type == "integer" %} int(11) unsigned
{%- else if type == "object" %} int(11) unsigned
{%- else if type == "money" %} decimal(10,2)
{%- else if type == "discount" %} decimal(10,3)
{%- else if type == "enum" %} enum({{ enumeration }})
{%- else if type == "boolean" %} tinyint(4)
{%- else if type == "date" %} date
{%- else if type == "time" %} time
{%- else if type == "datetime" %} datetime
{%- else if type == "timestamp" %} timestamp
{%- endif -%}
{%- if required %} NOT NULL{% endif -%}
{%- if default %} DEFAULT '{{ default }}'{% endif -%}
{%- if primary %} AUTO_INCREMENT, PRIMARY KEY (`{{ name }}`){% endif -%}
{%- if unique %} UNIQUE KEY `unique_{{ name }}` (`{{ name }}`){% endif -%}
{%- if index %} KEY `idx_{{ name }}` (`{{ name }}`){% endif -%}
{%- if comment %} -- {{ comment }}{% endif %}{% if ignore_last_field_sep or not loop.is_last %},{% endif %}
