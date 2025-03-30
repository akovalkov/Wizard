# Wizard	
Another template code generator 

## Build 
Build commands:
```
cmake -S . -B build
cmake --build  build --config Release
cmake --build  build --config Debug
```

## Template
Template syntax based on [Inja](https://github.com/pantor/inja) but with few changes.
The template inheritance (the "extends" and "block" statements) was removed
The following new tags were added:
### The file statement
Redirect the generator output to file
```
## file <output-path:expression>
... some output ... 
## endfile
```
The file statements can't be nested

### The apply-template statement
Process nested template with specified json data
```
## apply-template <template-name:string> <json field path:string>
```
## Template field constraints (optional)
Template fields may have strong typization through template description file
```
{
    "template": "DatabaseSchema",
    "description": "Template for db.sql, mysql dump",
    "variables": [
        {
            "name": "name",
            "type": "string",
            "required": true
        },
        {
            "name": "host",
            "type": "string",
            "default": "localhost"
        },
        {
            "name": "tables",
            "type": "array"
        }
    ]
}
```
The wizard will be check json field type and its presence during generation process. It helps for early error data type detection.
## Project (optional)
The project file allow to combine processing few templates using one source json data file


## CLI utility
```
wizard
Allowed options:
  --help                   produce help message
  -f [ --file ] arg        input template file
  -d [ --data ] arg        input JSON data file
  -i [ --info ] arg        template description (from json file)
  -c [ --create-info ] arg create/update template description (into json file)
  -o [ --output ] arg      output directory
  -p [ --project ] arg     input project file
```