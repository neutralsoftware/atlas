# Atlas Editor workflow

## Scene and project commands

| Command | Shortcut |
| --- | --- |
| New scene | Command N |
| Open scene | Command O |
| Save scene | Command S |
| Save scene as | Command Shift S |
| Close scene tab | Command W |
| Quit Atlas | Command Q |
| Project settings | Command , |
| Export project | File > Export Project |
| Build project | Command B |
| Run project | Command Shift B |

Scenes open as tabs above the viewport. Creating a scene uses the Atlas scene dialog and writes a new `.ascene` file. Project settings are stored inside the project at `.atlas/project-settings.ini`.

## Editing commands

| Command | Shortcut |
| --- | --- |
| Undo | Command Z |
| Redo | Command Shift Z |
| Cut, copy, paste | Command X, Command C, Command V |
| Duplicate selection | Command D |
| Delete selection | Backspace |
| Select all | Command A |
| Move, rotate, scale | G, R, S |
| Reset position | Command Option G |
| Reset rotation | Command Option R |
| Reset scale | Command Option S |
| Toggle local or world space | Shift T |
| Focus or frame selection | Tab |
| Pan viewport | Shift and drag |

G, R, and S start a modal transform. X, Y, and Z constrain axes; multiple axis keys combine constraints; Shift plus an axis excludes it. Enter or left click confirms. Escape or right click cancels. The pointer wraps around the viewport during modal transforms so the operation can continue without reaching a screen edge.

## Object and asset commands

| Command | Shortcut |
| --- | --- |
| Create empty | Shift N |
| Create camera | Shift C |
| Create point light | Shift L |
| Open Add Object | Shift A |
| Reparent | Shift R |
| Rename hierarchy object or asset | Enter |
| Search hierarchy | Command Shift F |
| Search assets | Command Option F |
| Refresh asset database | Command Shift R |

Shift-click selects multiple hierarchy objects. Dropping OBJ, FBX, glTF, GLB, or DAE files from the Content Browser onto the viewport imports a model.

## Runtime and discovery

| Command | Shortcut |
| --- | --- |
| Play or pause | Command P |
| Stop | Command Shift L |
| Step one frame | Command Shift K |
| Global project search | Command F |
| Command palette | Command Shift P |

The command palette lists available commands and their shortcuts and supports keyboard filtering, arrow navigation, and Enter. Scripts are watched for changes and the editor reloads the runtime automatically when JavaScript or TypeScript files change.
