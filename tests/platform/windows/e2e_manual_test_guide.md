# End-to-End Manual Test Guide

## Feature: windows-platform-support
## Task 13.2: 多应用测试

This guide describes manual testing procedures for verifying SuYan IME functionality across different applications.

## Prerequisites

1. SuYan IME is installed and registered
2. SuYanServer.exe is running
3. SuYan is selected as the active input method

## Test Applications

### 1. Notepad (记事本)

**Steps:**
1. Open Notepad (Win+R, type `notepad`, Enter)
2. Click in the text area to ensure focus
3. Type `nihao` - verify candidate window appears near cursor
4. Press `1` to select first candidate - verify Chinese text is inserted
5. Type `beijing` and press Space - verify first candidate is committed
6. Type `test` and press Escape - verify input is cancelled
7. Type `abc` and press Backspace twice - verify characters are deleted
8. Type `shi` and press PageDown/PageUp - verify page navigation works

**Expected Results:**
- [ ] Candidate window appears at cursor position
- [ ] Chinese characters are correctly inserted
- [ ] All key operations work as expected

### 2. Browser (浏览器)

**Test in address bar:**
1. Open Chrome/Edge/Firefox
2. Click in the address bar
3. Type `baidu` - verify candidate window appears
4. Press Escape to cancel
5. Type URL normally

**Test in text input:**
1. Navigate to a page with text input (e.g., search engine)
2. Click in the search box
3. Type `zhongguo` and select a candidate
4. Verify Chinese text appears in the input

**Expected Results:**
- [ ] Candidate window follows cursor in address bar
- [ ] Candidate window follows cursor in web page inputs
- [ ] Text is correctly committed to browser inputs

### 3. Word/WPS

**Steps:**
1. Open Microsoft Word or WPS Writer
2. Create a new document
3. Type `wenzhang` and select a candidate
4. Continue typing to create a paragraph
5. Test cursor movement and verify candidate window follows

**Expected Results:**
- [ ] Candidate window appears near text cursor
- [ ] Chinese text is correctly inserted into document
- [ ] Candidate window updates position when cursor moves

### 4. Terminal/Command Prompt

**Steps:**
1. Open Command Prompt or PowerShell
2. Type `echo ` followed by Chinese input
3. Verify candidate window appears
4. Select a candidate and verify text is inserted

**Expected Results:**
- [ ] Candidate window appears (may be at fixed position)
- [ ] Chinese text is correctly inserted

## Edge Cases

### Screen Edge Testing

1. Move application window to right edge of screen
2. Position cursor near right edge
3. Type pinyin - verify candidate window adjusts to stay on screen

### Multi-Monitor Testing

1. If multiple monitors available, move application to secondary monitor
2. Test input on secondary monitor
3. Verify candidate window appears on correct monitor

### Full-Screen Application

1. Open a full-screen application (e.g., full-screen browser)
2. Test input in full-screen mode
3. Verify candidate window appears above other content

## Test Checklist

| Test Case | Notepad | Browser | Word/WPS | Terminal |
|-----------|---------|---------|----------|----------|
| Candidate window appears | [ ] | [ ] | [ ] | [ ] |
| Window near cursor | [ ] | [ ] | [ ] | [ ] |
| Text commit works | [ ] | [ ] | [ ] | [ ] |
| Number selection | [ ] | [ ] | [ ] | [ ] |
| Space commit | [ ] | [ ] | [ ] | [ ] |
| Enter commit | [ ] | [ ] | [ ] | [ ] |
| Escape cancel | [ ] | [ ] | [ ] | [ ] |
| Backspace delete | [ ] | [ ] | [ ] | [ ] |
| Page navigation | [ ] | [ ] | [ ] | [ ] |

## Reporting Issues

When reporting issues, include:
1. Application name and version
2. Steps to reproduce
3. Expected behavior
4. Actual behavior
5. Screenshots if applicable
6. Log files from `%APPDATA%\SuYan\logs\`
