import simplepyble

if __name__ == "__main__":
    adapters = simplepyble.Adapter.get_adapters()

    if len(adapters) == 0:
        print("No adapters found")

    # Query the user to pick an adapter
    # print("Please select an adapter:")
    for i, adapter in enumerate(adapters):
        print(f"{i}: {adapter.identifier()} [{adapter.address()}]")

    # choice = int(input("Enter choice: "))
    adapter = adapters[0] # default to first adapter

    print(f"Selected adapter: {adapter.identifier()} [{adapter.address()}]")

    adapter.set_callback_on_scan_start(lambda: print("Scan started."))
    adapter.set_callback_on_scan_stop(lambda: print("Scan complete."))
    adapter.set_callback_on_scan_found(lambda peripheral: print(f"Found {peripheral.identifier()} [{peripheral.address()}]"))

    # Scan for 6 seconds
    adapter.scan_for(6000)
    peripherals = adapter.scan_get_results()

    # Query the user to pick a peripheral
    # print("Please select a peripheral:")
    # want step-seq_18E5 [D2793CE7-420F-5C3D-0524-B4A735AA2C81]
    peripherals = [p for p in peripherals if p.identifier() or p.address() == "D2793CE7-420F-5C3D-0524-B4A735AA2C81"]
    for i, peripheral in enumerate(peripherals):
        print(f"{i}: {peripheral.identifier()} [{peripheral.address()}]")
        if peripheral.identifier() == "step-seq_18E5" or peripheral.address() == "D2793CE7-420F-5C3D-0524-B4A735AA2C81":
            choice = i  

    if choice is None:
        print("step-seq_18E5 not found")
        exit(1)

    peripheral = peripherals[choice]

    print(f"Connecting to: {peripheral.identifier()} [{peripheral.address()}]")
    peripheral.connect()

    print("Successfully connected, listing services...")
    services = peripheral.services()
    service_characteristic_pair = []
    for service in services:
        for characteristic in service.characteristics():
            service_characteristic_pair.append((service.uuid(), characteristic.uuid()))

    # Query the user to pick a service/characteristic pair
    print("Please select a service/characteristic pair:")
    for i, (service_uuid, characteristic) in enumerate(service_characteristic_pair):
        print(f"{i}: {service_uuid} {characteristic}")

    while True:
        choice = int(input("Enter choice: "))
        service_uuid, characteristic_uuid = service_characteristic_pair[choice]

        try:    
            # Write the content to the characteristic
            contents = peripheral.read(service_uuid, characteristic_uuid)
            print(f"Contents: {contents}")
        except Exception as e:
            print(f"Error reading characteristic: {e}")
            continue

    peripheral.disconnect()
