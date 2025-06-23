## Vela: A PS Vita Chat Client for LLMs

Vela is a client for the PS Vita that allows you to connect and interact with LLMs.

<p float="left">
  <img src="https://github.com/user-attachments/assets/2f5e3ab1-b661-4aae-81b2-9c4b0ec9c488" width="45%" />
  <img src="https://github.com/user-attachments/assets/5939117d-4cc4-4e57-a559-39c7796f96da" width="45%" />
</p>


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


![chat_controls](https://github.com/user-attachments/assets/70701b21-fa79-4b72-9c7d-2a41ba170757)
![camera_controls](https://github.com/user-attachments/assets/9ad29a65-a9c6-4e76-a406-df9bca5488a3)
![chat_history_controls](https://github.com/user-attachments/assets/27eda19a-1bc9-46c7-9286-d9bcb99bca32)
![settings_controls](https://github.com/user-attachments/assets/c5c7aa9e-7843-47d4-85d0-1d1308f27f50)


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
