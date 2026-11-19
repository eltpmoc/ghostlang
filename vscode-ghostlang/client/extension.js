const path = require('path');
const vscode = require('vscode');
const { workspace } = vscode;
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

    // Depurador: 'ghost debug --dap' fala o Debug Adapter Protocol e controla o gdb
    context.subscriptions.push(vscode.debug.registerDebugAdapterDescriptorFactory('ghost', {
        createDebugAdapterDescriptor() {
            const ghost = workspace.getConfiguration('ghostlang').get('executablePath') || 'ghost';
            return new vscode.DebugAdapterExecutable(ghost, ['debug', '--dap']);
        }
    }));
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
