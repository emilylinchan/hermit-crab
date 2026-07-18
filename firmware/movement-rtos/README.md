Core 0                        Core 1
────────────────────          ────────────────────
webServerTask()               motionTask()
└── server.handleClient()     └── switch(currentCommand)
                                    └── gait functions
serialTask()                  
└── checkSerial()             
                              
All three read/write commandQueue to pass state