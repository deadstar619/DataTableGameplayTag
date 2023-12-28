# DataTable GameplayTag nodes for Unreal Engine 5.3

## Overview

The `DataTableGameplayTag` plugin adds the following nodes to the engine:

### Get Data Table Row By Tag

![Get Data Table Row By Tag](ExternalContent/GetDataTableRowByTag.png)

This node allows you to retrieve a row from a data table by specifying a gameplay tag instead of the row name. This eliminates the need to specify the gameplay tag twice in the data table.

### Get Data Table Row Tags

![Get Data Table Row Tags](ExternalContent/GetDataTableRowTags.png)

With this node, you can obtain all tags associated with the rows in a data table. Invalid tags will be logged.

## Installation
Clone or download the repository and place into the plugins folder.

I have only tested the plugin with 5.3 but should be working on 5.1.1 onwards.
