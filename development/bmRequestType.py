DIRECTION_OUT = 0
DIRECTION_IN = 1

TYPE_STANDARD = 0
TYPE_CLASS = 1
TYPE_VENDOR = 2
TYPE_RESERVED = 3

RECIPIENT_DEVICE = 0
RECIPIENT_INTERFACE = 1
RECIPIENT_ENDPOINT = 2
RECIPIENT_OTHER = 3

def createBmRequestType(direction, type, recipient):
    return ((direction << 7) | (type << 5) | recipient)

def reverseBmRequestType(bmRequestType):
    direction = (bmRequestType & 0x80) >> 7
    type = (bmRequestType & 0x60) >> 5
    recipient = (bmRequestType & 0x1f)
    directionStr = ""
    typeStr = ""
    recipientStr = ""

    if direction == DIRECTION_OUT:
        directionStr = "out"
    elif direction == DIRECTION_IN:
        directionStr = "in"
    else:
        directionStr = "unknown"

    if type == TYPE_STANDARD:
        typeStr = "standard"
    elif type == TYPE_CLASS:
        typeStr = "class"
    elif type == TYPE_VENDOR:
        typeStr = "vendor"
    elif type == TYPE_RESERVED:
        typeStr = "reserved"
    else:
        typeStr = "unknown"
    
    if recipient == RECIPIENT_DEVICE:
        recipientStr = "device"
    elif recipient == RECIPIENT_INTERFACE:
        recipientStr = "interface"
    elif recipient == RECIPIENT_ENDPOINT:
        recipientStr = "endpoint"
    elif recipient == RECIPIENT_OTHER:
        recipientStr = "other"
    else:
        recipientStr = "unknown"
    
    return (directionStr, typeStr, recipientStr)
    

def main():
    opt = input("Create or reverse bmRequestType? (c/r): ")
    if opt == "c":
        direction = input("Direction (in/out): ")
        type = input("Type (standard/class/vendor/reserved): ")
        recipient = input("Recipient (device/interface/endpoint/other): ")
        if direction == "in":
            direction = DIRECTION_IN
        elif direction == "out":
            direction = DIRECTION_OUT
        else:
            print("Invalid direction")
            return
        
        if type == "standard":
            type = TYPE_STANDARD
        elif type == "class":
            type = TYPE_CLASS
        elif type == "vendor":
            type = TYPE_VENDOR
        elif type == "reserved":
            type = TYPE_RESERVED
        else:
            print("Invalid type")
            return
        
        if recipient == "device":
            recipient = RECIPIENT_DEVICE
        elif recipient == "interface":
            recipient = RECIPIENT_INTERFACE
        elif recipient == "endpoint":
            recipient = RECIPIENT_ENDPOINT
        elif recipient == "other":
            recipient = RECIPIENT_OTHER
        else:
            print("Invalid recipient")
            return

        print("bmRequestType: 0x" + format(createBmRequestType(int(direction), int(type), int(recipient)), '02x'))
    elif opt == "r":
        bmRequestType = input("bmRequestType: ")
        if bmRequestType[:2] == "0x":
            bmRequestType = bmRequestType[2:]
        reversed = reverseBmRequestType(int(bmRequestType, 16))
        print("Direction: " + reversed[0])
        print("Type: " + reversed[1])
        print("Recipient: " + reversed[2])
    else:
        print("Invalid input")

main()