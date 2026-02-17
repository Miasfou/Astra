import fsp from 'fs/promises'; // fs/promises for async operations
import fs from 'fs'; // 'fs' for synchronous operations like createWriteStream
import path from 'path';
import { fileURLToPath } from 'url';

// --- Configuration ---
const config = {
    // The name of the file to be generated.
    outputFile: 'void_proj.txt',
    // Folders to completely ignore (tree and content).
    excludeFolders: [
        'third_party',
        '.idea',
        'node_modules',
        '.git',
        '.vscode',
        'dist',
        'build',
        'libs',
        'target'
    ],
    // Specific files to ignore (tree and content).
    excludeFiles: [
        'ui_data.hpp',
        'monocypher.h',
        'crypto.hpp',
        '.env',
        '.gitignore',
        'database.sqlite',
        'package-lock.json',
        'package.json',
    ],
    // File extensions to ignore (e.g., binaries, logs).
    excludeExtensions: [
        '.ttf',
        '.otf',
        '.mp3',
        '.wav',
        '.ogg',
        '.replay',
        '.bat',
        '.log',
        '.tmp',
        '.DS_Store',
        '.exe',
        '.dll',
        '.o',
        '.so',
        '.svg'
    ],
    // Extensions for files we assume are binary and shouldn't print content.
    binaryExtensions: [
        '.dat',
        '.txt',
        '.mp4',
        '.psd',
        '.png',
        '.jpg',
        '.jpeg',
        '.gif',
        '.ico',
        '.woff',
        '.woff2',
        '.ttf',
        '.eot',
        '.pdf',
        '.zip',
        '.gz',
        '.nbt',
    ],
};
// --- End Configuration ---


// --- Helper Functions ---

/**
 * Checks if a given path should be excluded based on the configuration.
 */
function isExcluded(entryName, exclusions) {
    if (exclusions.files.has(entryName)) return true;
    if (exclusions.folders.has(entryName)) return true;
    if (exclusions.extensions.has(path.extname(entryName))) return true;
    return false;
}
/**
 * Recursively generates a tree string for the directory structure.
 */
async function generateTree(dir, exclusions, prefix = '') {
    let entries;
    try {
        entries = await fsp.readdir(dir, { withFileTypes: true });
    } catch (error) {
        // Ignore errors for directories we can't read (e.g., permissions)
        return '';
    }

    let tree = '';

    // Filter out excluded entries
    const filteredEntries = entries.filter(entry => !isExcluded(entry.name, exclusions));

    for (let i = 0; i < filteredEntries.length; i++) {
        const entry = filteredEntries[i];
        const connector = i === filteredEntries.length - 1 ? '└── ' : '├── ';
        tree += `${prefix}${connector}${entry.name}\n`;

        if (entry.isDirectory()) {
            const newPrefix = prefix + (i === filteredEntries.length - 1 ? '    ' : '│   ');
            tree += await generateTree(path.join(dir, entry.name), exclusions, newPrefix);
        }
    }
    return tree;
}

/**
 * Recursively gets a flat list of all file paths to be included in the content dump.
 */
async function getFilePaths(dir, exclusions) {
    let files = [];
    let entries;
    try {
        entries = await fsp.readdir(dir, { withFileTypes: true });
    } catch (error) {
        return []; // Return empty if directory is unreadable
    }

    for (const entry of entries) {
        if (isExcluded(entry.name, exclusions)) {
            continue;
        }

        const fullPath = path.join(dir, entry.name);
        if (entry.isDirectory()) {
            files = files.concat(await getFilePaths(fullPath, exclusions));
        } else {
            files.push(fullPath);
        }
    }
    return files;
}


// --- Main execution with top-level await ---
try {
    console.log('Starting project export...');

    // In ESM, __filename is not available. This is the standard way to get it.
    const __filename = fileURLToPath(import.meta.url);
    const CWD = process.cwd();

    // Resolve absolute paths for the script and output file to ensure they are always excluded correctly.
    const scriptPath = path.resolve(__filename);
    const outputPath = path.resolve(CWD, config.outputFile);

    // Combine all exclusion criteria for easy checking.
    const exclusions = {
        folders: new Set(config.excludeFolders),
        files: new Set([...config.excludeFiles, path.basename(scriptPath), path.basename(outputPath)]),
        extensions: new Set(config.excludeExtensions),
        binaryExtensions: new Set(config.binaryExtensions),
    };

    // Create a writable stream to the output file.
    const outputStream = fs.createWriteStream(outputPath);
    const write = (data) => outputStream.write(data + '\n');

    // --- 1. Generate and write the tree structure ---
    write('tree:');
    write('');
    const tree = await generateTree(CWD, exclusions);
    write(tree);
    write('');

    // --- 2. Generate and write file contents ---
    write('--------------------------------------------------');
    write('File Contents:');
    write('--------------------------------------------------');
    write('');

    const filesToInclude = await getFilePaths(CWD, exclusions);

    for (const filePath of filesToInclude) {
        const relativePath = path.relative(CWD, filePath);
        write(`// --- File: ${relativePath} ---`);

        const fileExtension = path.extname(filePath);
        if (exclusions.binaryExtensions.has(fileExtension)) {
            write('// [Binary file - contents not displayed]');
        } else {
            const content = await fsp.readFile(filePath, 'utf8');
            outputStream.write(content);
        }
        write('\n'); // Add a newline for separation after file content
    }

    write('');
    write('--- Export Finished ---');

    outputStream.end();
    console.log(`Export complete. Output saved to "${config.outputFile}"`);

} catch (error) {
    console.error('An error occurred during the export:', error);
}