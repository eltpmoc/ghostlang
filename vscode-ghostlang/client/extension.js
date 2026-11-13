const path = require('path');
const { workspace, ExtensionContext } = require('vscode');
const { LanguageClient, LanguageClientOptions, ServerOptions } = require('vscode-languageclient/node');

let client;

function activate(context) {
    const serverCommand = workspace.getConfiguration('ghostlang').get('executablePath') || 'ghost';
    const serverArgs = ['lsp'];

    const serverOptions = {
        command: serverCommand,
        args: serverArgs,
        options: { shell: true }
    };

    const clientOptions = {
        documentSelector: [{ scheme: 'file', language: 'ghostlang' }],
        synchronize: {
            fileEvents: workspace.createFileSystemWatcher('**/*.{g,ghost}')
        }
    };

    client = new LanguageClient(
        'ghostlangLsp',
        'GhostLang Language Server',
        serverOptions,
        clientOptions
    );

    client.start();
}

function deactivate() {
    if (!client) {
        return undefined;
    }
    return client.stop();
}

module.exports = {
    activate,
    deactivate
};
