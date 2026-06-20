# RemoteUnrealParam

This is a dynamic wrapper for any and all types & classes.

> Whether the Remote or Local variant is used depends on the requirements of the data but the usage is identical with either param types.

## Inheritance
[RemoteObject](./remoteobject.md)

## Methods

### Get() / get()

- **Return type:** `auto`
- **Returns:** the underlying value for this param.

## set(auto NewValue)

- Sets the underlying value for this param.

## type()

- **Return type:** `string`
- **Returns:** "RemoteUnrealParam".

## Deleted Methods from inherited classes

### IsValid(), GetAddress()

- **From**: [RemoteObject](./remoteobject.md)
- **Reason**: A param can never be invalid, so this function is nonsensical.
- **Technical**: The lack of this function is due to a bug where RemoteUnrealObject doesn't correctly inherit from RemoteObject.  
Theoretically it wouldn't cause any problems to correct this, however, making a fundamental change to an important base class can cause headaches and maintenance burdens, so we should avoid it unless there's a good reason.