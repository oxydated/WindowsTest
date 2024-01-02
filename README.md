# TESTE APLICAÇÃO WINDOWS
# 
# 
### *LEANDRO ALMEIDA DE ARAUJO* ([oxydation@gmail.com])
#
### Source code disponível em: [WindowsTest]
# 
# 

IDE utilizado: VISUAL STUDIO COMMUNITY EDITION 2022 (Version 17.7.5)
Driver: [ODBC DB2 driver for Windows][Driver]
# 
# 

## EXECUTANDO AS TAREFAS

### HTTP REQUEST:
- No menu **HTTP** selecione **GET TEST**
-- Isso irá realizar uma requisição GET no [endereço fornecido][demo] e imprimirá o resultado no console integrado da aplicação.

### SQL QUERY:
- No menu **DB** selecione **Listar Feriados**
-- Isso irá listar o resultado da consulta no console integrado da aplicação.

### SQL PROCEDURE:
- No menu **DB** selecione **Verificar Feriado**
- Na caixa de diálogo que irá aparecer, selecione uma data e pressione o botão **OK**.
--A procedure *VERIFICARFERIADO* será executada e o status *true* ou *false* da data escolhida será impressa no console integrado da aplicação.

## OBSERVAÇÕES GERAIS:
- Na primeira vez em que um teste de ODBC for realizado (Query ou Procedure), uma caixa de diálogo surgirá para que o usuário escolha por um driver ODBC para usar.
--Isso foi feito dessa forma porque o **nome** do driver DB2 pode variar de máquina para máquina conforme a instalação.
- Apesar de apresentar todos os drivers ODBC disponíveis na máquina, **ESCOLHA SOMENTE O DRIVER DB2 DISPONÍVEL** para que a consulta ou a procedure sejam executadas dentro dos parâmetros solicitados pela tarefa.
- Todas as mensagens de erro de banco de dados ou HTTP são listadas no console.
-- É possível copiar e colar o conteúdo do console usando o menu do **botão direito** do mouse sobre o mesmo, em seguida usando "Selecionar Tudo" no mesmo menu e por fim usando "Copiar".

   [driver]: <https://www.ibm.com/docs/en/db2-big-sql/7.1?topic=drivers-odbc-driver-windows>
   [demo]: <http://demo5960524.mockable.io/>
   [oxydation@gmail.com]: <mailto:oxydation@gmail.com>
   [WindowsTest]: <https://github.com/oxydated/WindowsTest>
