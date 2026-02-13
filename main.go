package main

import (
	"fmt"
	"os"
	"strings"

	"github.com/gdamore/tcell/v2"
	"github.com/rivo/tview"
)

func parseCommandInput(input string) (string, []string) {
	split := strings.Split(input, " ")

	if len(split) >= 2 {
		return split[0], split[1:]

	} else if len(split) == 1 {
		return split[0], nil
	}

	return "", nil
}

func renderEditor(editorContent []string, cursorLine int, cursorColumn int) string {
	var result strings.Builder

	for i, line := range editorContent {
		coloured := line

		if i == cursorLine-1 {
			if cursorColumn-1 >= len(line) {
				coloured += "[black:white] [white:black]"

			} else {
				before := coloured[:cursorColumn-1]
				cursorChar := string(coloured[cursorColumn-1])
				after := coloured[cursorColumn:]

				coloured = before + "[black:white]" + cursorChar + "[white:black]" + after
			}
		}

		result.WriteString(coloured)

		if i < len(editorContent)-1 {
			result.WriteString("\n")
		}
	}

	return result.String()
}

func main() {
	app := tview.NewApplication()

	mainMenu := tview.NewTextView()
	mainMenu.SetTextColor(tcell.ColorWhite).SetTextAlign(tview.AlignCenter).SetText(
		`
CTRL+N to create new file
CTRL+Q to exit
`)
	mainMenu.SetBorder(true).SetBorderColor(tcell.ColorWhite).SetTitle("cupycode").SetTitleColor(tcell.ColorPink)

	editor := tview.NewTextView()
	editor.SetTextColor(tcell.ColorWhite).SetDynamicColors(true).SetTextAlign(tview.AlignLeft)
	editor.SetBorder(true).SetBorderColor(tcell.ColorPink).SetTitle("cupycode")
	editor.SetTextStyle(tcell.Style{})
	editor.SetBackgroundColor(tcell.ColorNone)
	editor.SetWrap(false)

	infoBox := tview.NewTextView()
	infoBox.SetTextColor(tcell.ColorWhite).SetTextAlign(tview.AlignLeft).SetText("No information to display ^^")
	infoBox.SetBorder(true).SetBorderColor(tcell.ColorGreen).SetTitle("Info")
	infoBox.SetBackgroundColor(tcell.ColorNone)

	commandBox := tview.NewTextArea()
	commandBox.SetBorder(true).SetBorderColor(tcell.ColorPurple).SetTitle("Commands")
	commandBox.SetBackgroundColor(tcell.ColorNone)

	editorFlex := tview.NewFlex().SetDirection(tview.FlexRow).AddItem(editor, 0, 20, true).AddItem(tview.NewFlex().AddItem(infoBox, 0, 1, false), 0, 2, false)

	pages := tview.NewPages()
	pages.AddPage("Menu", mainMenu, true, true)
	pages.AddPage("Editor", editorFlex, true, false)

	editorContent := []string{""}
	currentLine := 1
	currentColumn := 1

	editorOpen := false
	commandBoxOpen := false

	fileName := "cupycode"

	app.SetInputCapture(func(event *tcell.EventKey) *tcell.EventKey {
		// General

		if event.Key() == tcell.KeyCtrlQ {
			app.Stop()
		}

		// Editor

		if !editorOpen && event.Key() == tcell.KeyCtrlN {
			pages.SwitchToPage("Editor")
			editorOpen = true
		}

		if editor.HasFocus() && event.Key() == tcell.KeyEnter {
			// Insert new blank line undernearth current line
			editorContent = append(editorContent[:currentLine], append([]string{""}, editorContent[currentLine:]...)...)

			// If enter is pressed somewhere in the middle of the line (rather than the end)
			if currentColumn < len(editorContent[currentLine-1]) {
				remainingLineContent := editorContent[currentLine-1][currentColumn-1:]

				editorContent[currentLine-1] = editorContent[currentLine-1][:currentColumn-1]
				editorContent[currentLine] = remainingLineContent
			}

			currentLine++
			currentColumn = 1
		}

		if editor.HasFocus() && event.Key() == tcell.KeyRune {
			r := event.Rune()

			if r >= 32 && r <= 126 {
				line := editorContent[currentLine-1]

				if currentColumn-1 > len(line) {
					currentColumn = len(line) + 1
				}

				line = line[:currentColumn-1] + string(r) + line[currentColumn-1:]
				editorContent[currentLine-1] = line

				currentColumn++
			}
		}

		if editor.HasFocus() && (event.Key() == tcell.KeyTAB || event.Key() == tcell.KeyTab) {
			if currentColumn-1 >= len(editorContent[currentLine-1]) {
				editorContent[currentLine-1] += "    "

			} else {
				before := editorContent[currentLine-1][:currentColumn-1]
				after := editorContent[currentLine-1][currentColumn:]

				editorContent[currentLine-1] = before + "    " + after
			}

			currentColumn += 4
		}

		if editor.HasFocus() && event.Key() == tcell.KeyLeft {
			if currentColumn > 1 {
				currentColumn--

			} else if currentLine > 1 {
				currentLine--
				currentColumn = max(1, len(editorContent[currentLine-1])+1)
			}
		}

		if editor.HasFocus() && event.Key() == tcell.KeyRight {
			if currentColumn <= len(editorContent[currentLine-1]) {
				currentColumn++

			} else if currentLine < len(editorContent) {
				currentLine++
				currentColumn = 1
			}
		}

		if editor.HasFocus() && event.Key() == tcell.KeyUp {
			if currentLine > 1 {
				currentLine--
				if len(editorContent[currentLine-1]) > 0 {
					currentColumn = min(currentColumn, len(editorContent[currentLine-1]))
				}
			}
		}

		if editor.HasFocus() && event.Key() == tcell.KeyDown {
			if currentLine < len(editorContent) {
				currentLine++
			}
		}

		if editor.HasFocus() && (event.Key() == tcell.KeyBackspace || event.Key() == tcell.KeyBackspace2) {
			if currentColumn > 1 {
				line := editorContent[currentLine-1]
				line = line[:currentColumn-2] + line[currentColumn-1:]
				editorContent[currentLine-1] = line
				currentColumn--

			} else if currentLine > 1 {
				prevLineLen := len(editorContent[currentLine-2])
				editorContent[currentLine-2] += editorContent[currentLine-1]
				editorContent = append(editorContent[:currentLine-1], editorContent[currentLine:]...)
				currentLine--
				currentColumn = prevLineLen + 1
			}
		}

		if editorOpen && event.Key() == tcell.KeyCtrlC {
			if !commandBoxOpen {
				editorFlex = tview.NewFlex().SetDirection(tview.FlexRow).AddItem(editor, 0, 20, false).AddItem(tview.NewFlex().AddItem(commandBox, 0, 1, true), 0, 2, true)

			} else {
				editorFlex = tview.NewFlex().SetDirection(tview.FlexRow).AddItem(editor, 0, 20, true).AddItem(tview.NewFlex().AddItem(infoBox, 0, 1, true), 0, 2, false)
			}

			commandBoxOpen = !commandBoxOpen

			pages.RemovePage("Editor")
			pages.AddPage("Editor", editorFlex, true, true)

			return nil
		}

		// Command

		if commandBoxOpen && event.Key() == tcell.KeyEnter {
			input := strings.TrimSpace(commandBox.GetText())

			command, args := parseCommandInput(input)

			switch command {

			case "o", "open", "r", "read":
				if args == nil {
					infoBox.SetText("No path provided for open command")
					infoBox.SetBorderColor(tcell.ColorRed)

					break
				}

				path := args[0]

				fileContents, err := os.ReadFile(path)

				if err != nil {
					infoBox.SetText(err.Error())
					infoBox.SetBorderColor(tcell.ColorRed)

				} else {
					fileName = path

					editorContent = strings.Split(string(fileContents), "\n")
					currentLine = len(editorContent)
					currentColumn = len(editorContent[currentLine-1])

					infoBox.SetText("Successfully opened file " + path)
					infoBox.SetBorderColor(tcell.ColorGreen)
				}

			case "s", "save", "w", "write":
				var path string
				if fileName != "cupycode" {
					path = fileName
				} else {
					path = args[0]
				}

				err := os.WriteFile(path, []byte(editor.GetText(true)), 0666)

				if err != nil {
					infoBox.SetText(err.Error())
					infoBox.SetBorderColor(tcell.ColorRed)

				} else {
					fileName = path
					infoBox.SetText("Successfully written to file " + path)
					infoBox.SetBorderColor(tcell.ColorGreen)
				}

			case "q", "quit", "e", "exit":
				app.Stop()

			default:
				infoBox.SetText("Command \"" + command + "\" not recognised")
				infoBox.SetBorderColor(tcell.ColorRed)
			}

			commandBox.SetText("", false)
		}

		editor.SetTitle(fmt.Sprintf("%s @ (%d, %d)", fileName, currentLine, currentColumn))

		editor.SetText(renderEditor(editorContent, currentLine, currentColumn))

		return event
	})

	if err := app.SetRoot(pages, true).EnableMouse(true).Run(); err != nil {
		panic(err)
	}
}
