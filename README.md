## Vela: A PS Vita Chat Client for LLMs

Vela is a client for the PS Vita that allows you to connect and interact with LLMs.

### Features

*   **OpenAI-Compatible API Support**: Connect to any LLM endpoint that is OpenAI API Compatible.
*   **Image Messaging**: Send images to multimodal models directly from your PS Vita's camera. 
*   **Chat History and Sessions**: Conversations are saved, and you can create, delete, and switch between multiple chat sessions.


### Configuration

Vela saves its settings and chat sessions to the `ux0:data/vela/` directory on your PS Vita. You can configure your endpoints and API keys directly within the application's settings menu.

*   **Settings File**: `ux0:data/vela/settings.json`
*   **Sessions File**: `ux0:data/vela/sessions.json`
*   **Images Directory**: `ux0:data/vela/images/`

### Controls

[pending image graphic]


### Building from Source

To build Vela from source, you will need a working VitaSDK toolchain.

1.  **Clone the repository**:
    ```bash
    git clone https://github.com/callbacked/vela
    cd vela
    ```

2.  **Create a build directory**:
    ```bash
    mkdir build && cd build
    ```

3.  **Run CMake and build the project**:
    ```bash
    cmake .
    ```

4.  You'll see a `vela.vpk` file generated in your directory



---
