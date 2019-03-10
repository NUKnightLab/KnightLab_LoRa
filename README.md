# KnightLab_LoRa
Utilities for LoRa communication with Adafruit Feather M0 LoRa boards. Developed for the SensorGrid project.


## Development

### Unit testing

You must run a remote test server for unit tests to pass. Compile this project and load it onto a LoRa Feather M0. Be sure the test server's ID matches in the project code and the unit test code. This programmed feather will be your test server which must be running within radio range of the unit test device during testing.

Important: Be sure to always run more than just a single message transmission test. If a single test is run, the test server must be reset between test runs, otherwise you will thrash on the initial message ID which will result in opaquely rejected messages by the server.
