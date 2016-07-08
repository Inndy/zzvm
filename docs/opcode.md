## Byte Code Format

Every instruction are 4bytes-length, and there has 2 types of instruction:
`I-Type`, `R-Type`

The structure is descripted below:

### I-Type

```
[ 000XXXXX ] [ 0XXX ] [ 0XXX ] [ XXXXXXXXXXXXXXXX ]
```

| Length (bits) | Description     |
| ------------- | --------------- |
| 8             | Op code         |
| 4             | Register (dst)  |
| 4             | Register (src)  |
| 16            | Immidiate value |

### R-Type

```
[ 000XXXXX ] [ 0XXX ] [ 0XXX ] [ 0XXX ] [ 000000000000 ]
```

| Length (bits) | Description     |
| ------------- | --------------- |
| 8             | Op code         |
| 4             | Register (dst)  |
| 4             | Register (src)  |
| 4             | Register (src)  |
| 12            | Unused          |

## Registers

|  #  | Register Name | Description              |
| :-: | ------------- | ------------------------ |
|  0  | RA            | Return value             |
|  1  | R1            | General purpose register |
|  2  | R2            | General purpose register |
|  3  | R3            | General purpose register |
|  4  | R4            | General purpose register |
|  5  | R5            | General purpose register |
|  6  | SP            | Stack pointer            |
|  7  | IP            | Program counter          |


## OP Code List

See [zzvm/zzcode.h](../zzvm/zzcode.h).

## OP Code Reference

TBA, see [zz_execute in zzvm/zzvm.c](../zzvm/zzvm.c)
