{% file "companies.txt" %}
{% for company in companies -%}
{{ company.name }}
{% endfor -%}
{% endfile %}
{% for company in companies %}
{% for person in company.persons %}
{% file company.name + "/" + person + ".txt" %}
{{- loop.index }}:{{person -}}
{% endfile %}
{% endfor%}
{% endfor%}
